#include <pcap/pcap.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <network/network_interface.h>
#include <network/network_interface_pcap.h>

NetworkInterfacePCAP::NetworkInterfacePCAP(pcap_t *pcap)
: log_("/network/pcap"),
  pcap_(pcap),
  receive_callback_(NULL),
  receive_action_(NULL),
  transmit_callback_(NULL),
  transmit_action_(NULL)
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

	ASSERT(transmit_callback_ == NULL);
	ASSERT(transmit_action_ == NULL);
}

Action *
NetworkInterfacePCAP::receive(EventCallback *)
{
	NOTREACHED();
}

Action *
NetworkInterfacePCAP::transmit(Buffer *, EventCallback *)
{
	NOTREACHED();
}

void
NetworkInterfacePCAP::receive_callback(Event)
{
	NOTREACHED();
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
	NOTREACHED();
}

Action *
NetworkInterfacePCAP::receive_schedule(void)
{
	ASSERT(receive_action_ == NULL);

	EventCallback *cb = callback(this, &NetworkInterfacePCAP::receive_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Readable, pcap_fileno(pcap_), cb);
	return (a);
}

void
NetworkInterfacePCAP::transmit_callback(Event)
{
	NOTREACHED();
}

void
NetworkInterfacePCAP::transmit_cancel(void)
{
	if (transmit_callback_ != NULL) {
		delete transmit_callback_;
		transmit_callback_ = NULL;

		ASSERT(transmit_action_ != NULL);
	}

	if (transmit_action_ != NULL) {
		transmit_action_->cancel();
		transmit_action_ = NULL;
	}
}

Action *
NetworkInterfacePCAP::transmit_do(void)
{
	NOTREACHED();
}

Action *
NetworkInterfacePCAP::transmit_schedule(void)
{
	ASSERT(transmit_action_ == NULL);

	EventCallback *cb = callback(this, &NetworkInterfacePCAP::transmit_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Writable, pcap_fileno(pcap_), cb);
	return (a);
}

NetworkInterface *
NetworkInterfacePCAP::open(const std::string& ifname)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *pcap;

	errbuf[0] = '\0';
	pcap = pcap_open_live(ifname.c_str(), 65535, 1, 0, errbuf);
	if (pcap == NULL) {
		ERROR("/network/pcap") << "Unabled to open " << ifname << ": " << errbuf;
		return (NULL);
	}
	if (errbuf[0] != '\0')
		WARNING("/network/pcap") << "While opening " << ifname << ": " << errbuf;

	return (new NetworkInterfacePCAP(pcap));
}
