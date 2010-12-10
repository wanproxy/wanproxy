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

	ASSERT(receive_callback_ == NULL);
	ASSERT(receive_action_ == NULL);
}

Action *
NetworkInterfacePCAP::receive(EventCallback *cb)
{
	ASSERT(receive_action_ == NULL);
	ASSERT(receive_callback_ == NULL);

	receive_callback_ = cb;
	Action *a = receive_do();
	if (a == NULL) {
		ASSERT(receive_callback_ != NULL);
		receive_action_ = receive_schedule();
		ASSERT(receive_action_ != NULL);
		return (cancellation(this, &NetworkInterfacePCAP::receive_cancel));
	}
	ASSERT(receive_callback_ == NULL);
	return (a);
}

Action *
NetworkInterfacePCAP::transmit(Buffer *buffer, EventCallback *cb)
{
	ASSERT(!buffer->empty());
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
	ASSERT(receive_action_ != NULL);
}

void
NetworkInterfacePCAP::receive_cancel(void)
{
	if (receive_callback_ != NULL) {
		delete receive_callback_;
		receive_callback_ = NULL;

		ASSERT(receive_action_ != NULL);
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

	ASSERT(cnt == 1);

	receive_callback_->param(Event(Event::Done, packet));
	Action *a = receive_callback_->schedule();
	receive_callback_ = NULL;
	return (a);
}

Action *
NetworkInterfacePCAP::receive_schedule(void)
{
	ASSERT(receive_action_ == NULL);

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

	if (pcap_get_selectable_fd(pcap) == -1) {
		ERROR("/networ/pcap") << "Unable to get selectable file desciptor for " << ifname;
		pcap_close(pcap);
		return (NULL);
	}

	errbuf[0] = '\0';
	rv = pcap_setnonblock(pcap, 1, errbuf);
	if (rv == -1) {
		ERROR("/network/pcap") << "Could not set " << ifname << " to non-blocking mode: " << errbuf;
		pcap_close(pcap);
		return (NULL);
	}

	rv = pcap_set_promisc(pcap, 1);
	if (rv != 0) {
		ERROR("/network/pcap") << "Could not put " << ifname << " into promiscuous mode: " << pcap_geterr(pcap);
		pcap_close(pcap);
		return (NULL);
	}

	rv = pcap_set_buffer_size(pcap, 65536);
	ASSERT(rv == 0);

	rv = pcap_activate(pcap);
	if (rv != 0) {
		ERROR("/network/pcap") << "Could not activate " << ifname << ": " << pcap_geterr(pcap);
		pcap_close(pcap);
		return (NULL);
	}

	return (new NetworkInterfacePCAP(pcap));
}

static void
network_interface_pcap_dispatch(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)
{
	Buffer *packet = (Buffer *)user;
	ASSERT(packet->empty());

	packet->append(bytes, h->caplen);
}
