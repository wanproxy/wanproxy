/*
 * Copyright (c) 2010-2012 Juli Mallett. All rights reserved.
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

#include <pcap/pcap.h>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <network/network_interface.h>
#include <network/network_interface_pcap.h>

static void network_interface_pcap_dispatch(u_char *, const struct pcap_pkthdr *, const u_char *);

NetworkInterfacePCAP::NetworkInterfacePCAP(pcap_t *pcap)
: log_("/network/pcap"),
  pcap_(pcap),
  receive_callback_(NULL),
  receive_action_(NULL)
{ }

NetworkInterfacePCAP::~NetworkInterfacePCAP()
{
	if (pcap_ != NULL) {
		pcap_close(pcap_);
		pcap_ = NULL;
	}

	ASSERT(log_, receive_callback_ == NULL);
	ASSERT(log_, receive_action_ == NULL);
}

Action *
NetworkInterfacePCAP::receive(EventCallback *cb)
{
	ASSERT(log_, receive_action_ == NULL);
	ASSERT(log_, receive_callback_ == NULL);

	receive_callback_ = cb;
	Action *a = receive_do();
	if (a == NULL) {
		ASSERT(log_, receive_callback_ != NULL);
		receive_action_ = receive_schedule();
		ASSERT(log_, receive_action_ != NULL);
		return (cancellation(this, &NetworkInterfacePCAP::receive_cancel));
	}
	ASSERT(log_, receive_callback_ == NULL);
	return (a);
}

Action *
NetworkInterfacePCAP::transmit(Buffer *buffer, EventCallback *cb)
{
	ASSERT(log_, !buffer->empty());
	buffer->clear();

	cb->param(Event::Error);
	return (cb->schedule());
}

void
NetworkInterfacePCAP::receive_callback(Event e)
{
	receive_action_->cancel();
	receive_action_ = NULL;

	switch (e.type_) {
	case Event::EOS:
	case Event::Done:
		break;
	case Event::Error: {
		DEBUG(log_) << "Poll returned error: " << e;
		receive_callback_->param(e);
		Action *a = receive_callback_->schedule();
		receive_action_ = a;
		receive_callback_ = NULL;
		return;
	}
	default:
		HALT(log_) << "Unexpected event: " << e;
	}

	receive_action_ = receive_do();
	if (receive_action_ == NULL)
		receive_action_ = receive_schedule();
	ASSERT(log_, receive_action_ != NULL);
}

void
NetworkInterfacePCAP::receive_cancel(void)
{
	if (receive_callback_ != NULL) {
		delete receive_callback_;
		receive_callback_ = NULL;

		ASSERT(log_, receive_action_ != NULL);
	}

	if (receive_action_ != NULL) {
		receive_action_->cancel();
		receive_action_ = NULL;
	}
}

Action *
NetworkInterfacePCAP::receive_do(void)
{
	Buffer packet;
	int cnt = pcap_dispatch(pcap_, 1, network_interface_pcap_dispatch, (u_char *)&packet);
	if (cnt == -1) {
		receive_callback_->param(Event(Event::Error));
		Action *a = receive_callback_->schedule();
		receive_callback_ = NULL;
		return (a);
	}

	if (cnt == 0) {
		return (NULL);
	}

	ASSERT(log_, cnt == 1);

	receive_callback_->param(Event(Event::Done, packet));
	Action *a = receive_callback_->schedule();
	receive_callback_ = NULL;
	return (a);
}

Action *
NetworkInterfacePCAP::receive_schedule(void)
{
	ASSERT(log_, receive_action_ == NULL);

	EventCallback *cb = callback(this, &NetworkInterfacePCAP::receive_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Readable, pcap_get_selectable_fd(pcap_), cb);
	return (a);
}

NetworkInterface *
NetworkInterfacePCAP::open(const std::string& ifname)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *pcap;
	int rv;

	errbuf[0] = '\0';
	pcap = pcap_create(ifname.c_str(), errbuf);
	if (pcap == NULL) {
		ERROR("/network/pcap") << "Unabled to create " << ifname << ": " << errbuf;
		return (NULL);
	}

	errbuf[0] = '\0';
	rv = pcap_setnonblock(pcap, 1, errbuf);
	if (rv == -1) {
		ERROR("/network/pcap") << "Could not set " << ifname << " to non-blocking mode: " << errbuf;
		pcap_close(pcap);
		return (NULL);
	}

	rv = pcap_set_snaplen(pcap, 65535);
	if (rv != 0) {
		ERROR("/network/pcap") << "Could not set snap length for " << ifname << ": " << pcap_geterr(pcap);
		pcap_close(pcap);
		return (NULL);
	}

	rv = pcap_set_promisc(pcap, 1);
	if (rv != 0) {
		ERROR("/network/pcap") << "Could not put " << ifname << " into promiscuous mode: " << pcap_geterr(pcap);
		pcap_close(pcap);
		return (NULL);
	}

	rv = pcap_set_timeout(pcap, 1 /* ms */);
	if (rv != 0) {
		ERROR("/network/pcap") << "Could not set timeout for " << ifname << ": " << pcap_geterr(pcap);
		pcap_close(pcap);
		return (NULL);
	}

	rv = pcap_set_buffer_size(pcap, 65536);
	ASSERT("/network/pcap", rv == 0);

	rv = pcap_activate(pcap);
	if (rv != 0) {
		ERROR("/network/pcap") << "Could not activate " << ifname << ": " << pcap_geterr(pcap);
		pcap_close(pcap);
		return (NULL);
	}

	if (pcap_get_selectable_fd(pcap) == -1) {
		ERROR("/network/pcap") << "Unable to get selectable file desciptor for " << ifname;
		pcap_close(pcap);
		return (NULL);
	}

	return (new NetworkInterfacePCAP(pcap));
}

static void
network_interface_pcap_dispatch(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)
{
	Buffer *packet = (Buffer *)(void *)user;
	ASSERT("/network/pcap/dispatch", packet->empty());

	packet->append(bytes, h->caplen);
}
