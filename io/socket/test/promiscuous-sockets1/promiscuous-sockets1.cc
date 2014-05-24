/*
 * Copyright (c) 2013 Patrick Kelsey. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Seems at least some versions of clang++ put unordered_map in the std
 * namespace even if building for something earlier than c++11.
 */ 
#ifndef FORCE_CXX11_OR_LATER
#define FORCE_CXX11_OR_LATER 1
#endif

#if (__cplusplus > 199711L) || FORCE_CXX11_OR_LATER
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif

#include <common/buffer.h>
#include <common/test.h>

#include <event/event_callback.h>
#include <event/typed_callback.h>
#include <event/event_main.h>

#include <io/pipe/splice.h>
#include <io/pipe/splice_pair.h>
#include <io/socket/resolver.h>
#include <io/socket/socket_uinet_promisc.h>
#include <io/io_uinet.h>

#if (__cplusplus > 199711L) || FORCE_CXX11_OR_LATER
namespace umns {
	using namespace ::std;
};
#else
namespace umns {
	using namespace ::std::tr1;
};
#endif

class Listener {
	struct ConnInfo {
		struct uinet_in_conninfo inc;
		struct uinet_in_l2info l2i;

		bool operator== (const ConnInfo& other) const
		{
			if ((inc.inc_fibnum != other.inc.inc_fibnum) ||
			    (inc.inc_ie.ie_laddr.s_addr != other.inc.inc_ie.ie_laddr.s_addr) ||
			    (inc.inc_ie.ie_lport != other.inc.inc_ie.ie_lport) ||
			    (inc.inc_ie.ie_faddr.s_addr != other.inc.inc_ie.ie_faddr.s_addr) ||
			    (inc.inc_ie.ie_fport != other.inc.inc_ie.ie_fport))
				return (false);

			if (0 != uinet_l2tagstack_cmp(&l2i.inl2i_tagstack, &other.l2i.inl2i_tagstack))
				return (false);

			return (true);
		}
	};

	struct SpliceInfo {
		ConnInfo conninfo;
		uinet_synf_deferral_t deferral;
		Action *connect_action;
		SocketUinetPromisc *inbound;
		SocketUinetPromisc *outbound;
		Splice *splice_inbound;
		Splice *splice_outbound;
		SplicePair *splice_pair;
		Action *splice_action;
		Action *close_action;
	};

	struct ConnInfoHasher {
		std::size_t operator()(const ConnInfo& ci) const
		{
			std::size_t hash;
			
			hash =
			    (ci.inc.inc_fibnum << 11) ^
			    ci.inc.inc_ie.ie_laddr.s_addr ^
			    ci.inc.inc_ie.ie_lport ^
			    ci.inc.inc_ie.ie_faddr.s_addr ^
			    ci.inc.inc_ie.ie_fport;
			
			hash ^= uinet_l2tagstack_hash(&ci.l2i.inl2i_tagstack);
			
			return (hash);
		} 
	};

	LogHandle log_;
	SocketUinetPromisc *listen_socket_;
	Mutex mtx_;
	Action *action_;
	unsigned int inbound_cdom_;
	unsigned int outbound_cdom_;
	SocketUinetPromisc::SynfilterCallback *synfilter_callback_;

	umns::unordered_map<ConnInfo,SpliceInfo,ConnInfoHasher> connections_;

public:
	Listener(const std::string& where, unsigned int inbound_cdom, unsigned int outbound_cdom)
	: log_("/listener"),
	  mtx_("Listener"),
	  action_(NULL),
	  inbound_cdom_(inbound_cdom),
	  outbound_cdom_(outbound_cdom)
	{
		synfilter_callback_ = callback(this, &Listener::synfilter);

		listen_socket_ = SocketUinetPromisc::create(SocketAddressFamilyIPv4, SocketTypeStream, "tcp", "", inbound_cdom_);
		if (listen_socket_ == NULL)
			return;

		listen_socket_->setsynfilter(synfilter_callback_);

		if (0 != listen_socket_->setl2info2(NULL, NULL, UINET_INL2I_TAG_ANY, NULL))
			return;

		if (!listen_socket_->bind(where))
			return;	

		if (!listen_socket_->listen())
			return;

		{
			ScopedLock _(&mtx_);
			
			SocketEventCallback *cb = callback(this, &Listener::accept_complete);
			action_ = listen_socket_->accept(cb);
		}
	}

	~Listener()
	{
		ScopedLock _(&mtx_);

		if (action_ != NULL)
			action_->cancel();
		action_ = NULL;

		if (listen_socket_ != NULL)
			delete listen_socket_;
		listen_socket_ = NULL;
	}

	
	/*
	 * This will get invoked for every SYN received on listen_socket_.
	 * The connection details for the SYN are tracked in the
	 * connections_ table for two reasons.  The first is that we only
	 * want to take action (that is, open an outbound connection to the
	 * destination) for a given SYN once, not each time it may be
	 * retransmitted.  The second is that when a SYN is accepted by the
	 * filter, the stack then processes it in the normal path, resulting
	 * in a subsequent accept, and that accept has to be able to locate
	 * the outbound connection made by the filter and associate it with
	 * the inbound connection being accepted.  There is no way to pass
	 * any context to that subsequent accept from this syn filter
	 * through the stack (because, syncookies).
	 */
	void synfilter(SocketUinetPromisc::SynfilterCallbackParam param)
	{
		ScopedLock _(&mtx_);

		ConnInfo ci;
		ci.inc = *param.inc;
		ci.l2i = *param.l2i;

		if (connections_.find(ci) == connections_.end()) {

			struct uinet_in_l2info *l2i = param.l2i;

			SpliceInfo& si = connections_[ci];
			si.conninfo = ci;

			/*
			 * Get deferral context for the SYN filter result.
			 */
			si.deferral = listen_socket_->synfdefer(param.cookie);

			/*
			 * Set up outbound connection.
			 */
			si.inbound = NULL;
			si.outbound = SocketUinetPromisc::create(SocketAddressFamilyIPv4, SocketTypeStream, "tcp", "", outbound_cdom_);
			if (NULL == si.outbound)
				goto out;

			/*
			 * Use the L2 details of the foreign host that sent this SYN.
			 */
			si.outbound->setl2info2(l2i->inl2i_foreign_addr, l2i->inl2i_local_addr,
						l2i->inl2i_flags, &l2i->inl2i_tagstack);

			/*
			 * Use the IP address and port number of the foreign
			 * host that sent this SYN.
			 */
			/* XXX perhaps resolver could support us a bit better here */
			socket_address addr1;
			addr1.addrlen_ = sizeof(struct sockaddr_in);
/* XXX */
#if !defined(__linux__)
			addr1.addr_.inet_.sin_len = addr1.addrlen_;
#endif
			addr1.addr_.inet_.sin_family = AF_INET; 
			addr1.addr_.inet_.sin_port = ci.inc.inc_ie.ie_fport; 
			addr1.addr_.inet_.sin_addr.s_addr = ci.inc.inc_ie.ie_faddr.s_addr;
			si.outbound->bind(addr1);
			
			/*
			 * Connect to the same IP address and port number
			 * that the foreign host that sent this SYN is
			 * trying to connect to.  On connection completion,
			 * the deferred SYN filter decision will be
			 * delivered.  Once the deferred decision is
			 * delivered, and if it is UINET_SYNF_ACCEPT, the
			 * accept for the original inbound connection will
			 * complete.
			 */
			EventCallback *cb = callback(this, &Listener::connect_complete, ci);
			socket_address addr2;
			addr2.addrlen_ = sizeof(struct sockaddr_in);
/* XXX */
#if !defined(__linux__)
                        addr2.addr_.inet_.sin_len = addr2.addrlen_;
#endif
			addr2.addr_.inet_.sin_family = AF_INET; 
			addr2.addr_.inet_.sin_port = ci.inc.inc_ie.ie_lport; 
			addr2.addr_.inet_.sin_addr.s_addr = ci.inc.inc_ie.ie_laddr.s_addr;
			si.connect_action = si.outbound->connect(addr2, cb);

			DEBUG(log_) << "Outbound from " << (std::string)addr1 << " to " << (std::string)addr2;

			*param.result = UINET_SYNF_DEFER;

			return;
		}

	out:
		*param.result = UINET_SYNF_REJECT_SILENT;
	}

	void accept_complete(Event e, Socket *socket)
	{
		ScopedLock _(&mtx_);
			
		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done: {

			DEBUG(log_) << "Inbound from " << socket->getpeername() << " to " << socket->getsockname();
			
			SocketUinetPromisc *promisc = (SocketUinetPromisc *)socket;
			ConnInfo ci;

			promisc->getconninfo(&ci.inc);
			promisc->getl2info(&ci.l2i);

			SpliceInfo& si = connections_[ci];
			si.inbound = promisc;
			si.splice_inbound = new Splice(log_ + "/inbound", si.inbound, NULL, si.outbound);
			si.splice_outbound = new Splice(log_ + "/outbound", si.outbound, NULL, si.inbound);
			si.splice_pair = new SplicePair(si.splice_inbound, si.splice_outbound);
			EventCallback *cb = callback(this, &Listener::splice_complete, ci);
			si.splice_action = si.splice_pair->start(cb);

			break;
		}
		default:
			ERROR(log_) << "Unexpected event: " << e;
			return;
		}

		SocketEventCallback *cb = callback(this, &Listener::accept_complete);
		action_ = listen_socket_->accept(cb);
	}

	void connect_complete(Event e, ConnInfo ci)
	{
		ScopedLock _(&mtx_);
		SpliceInfo& si = connections_[ci];

		si.connect_action->cancel();
		si.connect_action = NULL;

		switch (e.type_) {
		case Event::Done:
			listen_socket_->synfdeferraldeliver(si.deferral, UINET_SYNF_ACCEPT);
			break;
		default:
			connections_.erase(ci);
			listen_socket_->synfdeferraldeliver(si.deferral, UINET_SYNF_REJECT_RST);
			/* XXX Close socket?  */
			ERROR(log_) << "Unexpected event: " << e;
			break;
		}

	}

	void splice_complete(Event e, ConnInfo ci)
	{
		ScopedLock _(&mtx_);
		SpliceInfo& si = connections_[ci];
		si.splice_action->cancel();
		si.splice_action = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		default:
			ERROR(log_) << "Splice exiting unexpectedly: " << e;
			break;
		}

		ASSERT(log_, si.splice_pair != NULL);
		delete si.splice_pair;
		si.splice_pair = NULL;

		ASSERT(log_, si.splice_inbound != NULL);
		delete si.splice_inbound;
		si.splice_inbound = NULL;

		ASSERT(log_, si.splice_outbound != NULL);
		delete si.splice_outbound;
		si.splice_outbound = NULL;

		SimpleCallback *cb = callback(this, &Listener::close_complete, ci);
		si.close_action = si.outbound->close(cb);
	}

	void close_complete(ConnInfo ci)
	{
		ScopedLock _(&mtx_);
		SpliceInfo& si = connections_[ci];
		si.close_action->cancel();
		si.close_action = NULL;

		if (si.outbound != NULL) {
			delete si.outbound;
			si.outbound = NULL;

			SimpleCallback *cb = callback(this, &Listener::close_complete, ci);
			si.close_action = si.inbound->close(cb);
			return;
		}

		delete si.inbound;
		si.inbound = NULL;

		connections_.erase(ci);
	}
};

int
main(void)
{
	IOUinet::instance()->start(false);

	IOUinet::instance()->add_interface(UINET_IFTYPE_NETMAP, "vale0:1", "proxy-a-side", 1, -1);
	IOUinet::instance()->interface_up("proxy-a-side", true, true);

	IOUinet::instance()->add_interface(UINET_IFTYPE_NETMAP, "vale1:1", "proxy-b-side", 2, -1);
	IOUinet::instance()->interface_up("proxy-b-side", true, true);

	Listener *l = new Listener("[0.0.0.0]:0", 1, 2);

	event_main();

#if notquiteyet
	IOUinet::instance()->remove_interface("proxy-a-side");
	IOUinet::instance()->remove_interface("proxy-b-side");
#endif

	delete l;
}
