#include <pcap/pcap.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <network/network_interface.h>
#include <network/network_interface_pcap.h>

/*
 * XXX
 * This doesn't actually not block.  Argh!
 */

NetworkInterfacePCAP::NetworkInterfacePCAP(pcap_t *pcap)
: log_("/network/pcap"),
  pcap_(pcap),
  receive_callback_(NULL),
  receive_action_(NULL)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	int rv;

	errbuf[0] = '\0';
	rv = pcap_setnonblock(pcap_, 1, errbuf);
	if (rv == -1)
		ERROR("/network/pcap") << "Could not set interface to non-blocking mode.";
}

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
	return (EventSystem::instance()->schedule(cb));
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
		Action *a = EventSystem::instance()->schedule(receive_callback_);
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
	struct pcap_pkthdr *h;
	const u_char *bytes;
	int rv = pcap_next_ex(pcap_, &h, &bytes);
	if (rv == -1) {
		receive_callback_->param(Event(Event::Error));
		Action *a = EventSystem::instance()->schedule(receive_callback_);
		receive_callback_ = NULL;
		return (a);
	}

	if (rv == 0) {
		return (NULL);
	}

	ASSERT(rv == 1);

	receive_callback_->param(Event(Event::Done, Buffer(bytes, h->caplen)));
	Action *a = EventSystem::instance()->schedule(receive_callback_);
	receive_callback_ = NULL;
	return (a);
}

Action *
NetworkInterfacePCAP::receive_schedule(void)
{
	ASSERT(receive_action_ == NULL);

	EventCallback *cb = callback(this, &NetworkInterfacePCAP::receive_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Readable, pcap_fileno(pcap_), cb);
	return (a);
}

NetworkInterface *
NetworkInterfacePCAP::open(const std::string& ifname)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *pcap;

	errbuf[0] = '\0';
	pcap = pcap_open_live(ifname.c_str(), 65535, 1, 1, errbuf);
	if (pcap == NULL) {
		ERROR("/network/pcap") << "Unabled to open " << ifname << ": " << errbuf;
		return (NULL);
	}
	if (errbuf[0] != '\0')
		WARNING("/network/pcap") << "While opening " << ifname << ": " << errbuf;

	return (new NetworkInterfacePCAP(pcap));
}
