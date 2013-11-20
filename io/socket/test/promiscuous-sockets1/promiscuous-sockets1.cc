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

#include <tr1/unordered_map>

#include <common/buffer.h>
#include <common/test.h>

#include <event/event_callback.h>
#include <event/typed_callback.h>
#include <event/event_main.h>

#include <io/socket/resolver.h>
#include <io/socket/socket_uinet_promisc.h>
#include <io/io_uinet.h>



class Connector {
	LogHandle log_;
	Mutex mtx_;
	Action *action_;
	Socket *readfrom_;
	Socket *writeto_;
	Buffer read_buffer_;

public:
	Connector(Socket *readfrom, Socket *writeto)
	: log_("/connector"),
	  mtx_("Connector"),
	  action_(NULL),
	  readfrom_(readfrom),
	  writeto_(writeto),
	  read_buffer_()
	{

		{
			ScopedLock _(&mtx_);
			
			EventCallback *cb = callback(this, &Connector::read_complete);
			action_ = readfrom_->read(0, cb);
		}
	}

	~Connector()
	{
	}

	void read_complete(Event e)
	{
		ScopedLock _(&mtx_);

		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			read_buffer_.append(e.buffer_);
			break;
		case Event::EOS:
			read_buffer_.append(e.buffer_);
			DEBUG(log_) << "Read EOS";
			break;
		default:
			ERROR(log_) << "Unexpected event: " << e;
			return;
		}

		if (!read_buffer_.empty()) {
			EventCallback *cb = callback(this, &Connector::write_complete);
			action_ = writeto_->write(&read_buffer_, cb);
		}
	}
	
	void write_complete(Event e)
	{
		/* XXX lock probably not necessary since this is launched
		 * from the same event thread that will process it.
		 */
		ScopedLock _(&mtx_);

		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		default:
			ERROR(log_) << "Unexpected event: " << e;
			return;
		}

		EventCallback *cb = callback(this, &Connector::read_complete);
		action_ = readfrom_->read(0, cb);
	}

};


class Splice {
	LogHandle log_;
	Connector inbound_to_outbound_;
	Connector outbound_to_inbound_;

public:
	Splice(Socket *inbound, Socket *outbound)
	: log_("/splice"),
	  inbound_to_outbound_(inbound, outbound),
	  outbound_to_inbound_(outbound, inbound)
	{
	}

	~Splice()
	{
	}
};



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

			/* XXX this comparison should be provided by UINET */
			if ((l2i.inl2i_cnt != other.l2i.inl2i_cnt) ||
			    ((l2i.inl2i_cnt > 0) &&
			     (l2i.inl2i_mask != other.l2i.inl2i_mask)))
				return (false);

			for (int i = 0; i < l2i.inl2i_cnt; i++) {
				if ((l2i.inl2i_tags[i] & l2i.inl2i_mask) !=
				    (other.l2i.inl2i_tags[i] & other.l2i.inl2i_mask))
					return (false);
			}

			return (true);
		}
	};

	struct SpliceInfo {
		uinet_synf_deferral_t deferral;
		Action *connect_action;
		SocketUinetPromisc *inbound;
		SocketUinetPromisc *outbound;
		Splice *splice;
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
			    ci.inc.inc_ie.ie_fport ^
			    ci.l2i.inl2i_cnt;
			
			for (int i = 0; i < ci.l2i.inl2i_cnt; i++) {
				hash ^= ci.l2i.inl2i_tags[i] & ci.l2i.inl2i_mask;
			}
			
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

	std::tr1::unordered_map<ConnInfo,SpliceInfo,ConnInfoHasher> connections_;


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

			SpliceInfo si;
			struct uinet_in_l2info *l2i = param.l2i;

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
			/* It's OK to modify the l2info whose pointer was given to us in param. */
			/* XXX maybe provide the old setl2info api again
			 * that takes the items individually, in order to
			 * minmize copies */
			uint8_t temp_addr[UINET_IN_L2INFO_ADDR_MAX];
			memcpy(temp_addr, l2i->inl2i_local_addr, UINET_IN_L2INFO_ADDR_MAX);
			memcpy(l2i->inl2i_local_addr, l2i->inl2i_foreign_addr, UINET_IN_L2INFO_ADDR_MAX);
			memcpy(l2i->inl2i_foreign_addr, temp_addr, UINET_IN_L2INFO_ADDR_MAX);

			si.outbound->setl2info(l2i);

			/*
			 * Use the IP address and port number of the foreign
			 * host that sent this SYN.
			 */
			/* XXX perhaps resolver could support us a bit better here */
			socket_address addr1;
			addr1.addr_.inet_.sin_len = sizeof(struct sockaddr_in); 
			addr1.addr_.inet_.sin_family = AF_INET; 
			addr1.addr_.inet_.sin_port = ci.inc.inc_ie.ie_fport; 
			addr1.addr_.inet_.sin_addr.s_addr = ci.inc.inc_ie.ie_faddr.s_addr;
			addr1.addrlen_ = addr1.addr_.inet_.sin_len;
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
			addr2.addr_.inet_.sin_len = sizeof(struct sockaddr_in); 
			addr2.addr_.inet_.sin_family = AF_INET; 
			addr2.addr_.inet_.sin_port = ci.inc.inc_ie.ie_lport; 
			addr2.addr_.inet_.sin_addr.s_addr = ci.inc.inc_ie.ie_laddr.s_addr;
			addr2.addrlen_ = addr2.addr_.inet_.sin_len;
			si.connect_action = si.outbound->connect(addr2, cb);

			DEBUG(log_) << "Outbound from " << (std::string)addr1 << " to " << (std::string)addr2;

			*param.result = UINET_SYNF_DEFER;

			connections_[ci] = si;

			return;
		}

	out:
		*param.result = UINET_SYNF_REJECT;
	}

	void accept_complete(Event e, Socket *socket)
	{

		ScopedLock _(&mtx_);
			
		(void)socket;

		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done: {

			DEBUG(log_) << "Inbound from " << socket->getpeername() << " to " << socket->getsockname();
			
			SocketUinetPromisc *promisc = (SocketUinetPromisc *)socket;
			ConnInfo ci;
			SpliceInfo si;

			promisc->getconninfo(&ci.inc);
			promisc->getl2info(&ci.l2i);

			si = connections_[ci];
			
			si.inbound = promisc;
			si.splice = new Splice(si.inbound, si.outbound);

			connections_[ci] = si;

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

		SpliceInfo si = connections_[ci];

		si.connect_action->cancel();
		si.connect_action = NULL;

		switch (e.type_) {
		case Event::Done:
			listen_socket_->synfdeferraldeliver(si.deferral, UINET_SYNF_ACCEPT);
			break;
		default:
			connections_.erase(ci);
			listen_socket_->synfdeferraldeliver(si.deferral, UINET_SYNF_REJECT);
			ERROR(log_) << "Unexpected event: " << e;
			break;
		}

	}
};


int
main(void)
{
	IOUinet::instance()->add_interface("em1", UINET_IFTYPE_NETMAP, 1, -1);
	IOUinet::instance()->add_interface("em2", UINET_IFTYPE_NETMAP, 2, -1);
	IOUinet::instance()->start(false);


	Listener *l = new Listener("[0.0.0.0]:0", 1, 2);

	event_main();

	delete l;
}
