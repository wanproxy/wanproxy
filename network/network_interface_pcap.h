#ifndef	NETWORK_NETWORK_INTERFACE_PCAP_H
#define	NETWORK_NETWORK_INTERFACE_PCAP_H

struct pcap;

typedef	struct pcap pcap_t;

class NetworkInterfacePCAP : public NetworkInterface {
	LogHandle log_;
	pcap_t *pcap_;

	EventCallback *receive_callback_;
	Action *receive_action_;
private:
	NetworkInterfacePCAP(pcap_t *);

public:
	~NetworkInterfacePCAP();

	Action *receive(EventCallback *);
	Action *transmit(Buffer *, EventCallback *);

private:
	void receive_callback(Event);
	void receive_cancel(void);
	Action *receive_do(void);
	Action *receive_schedule(void);

public:
	static NetworkInterface *open(const std::string&);
};

#endif /* !NETWORK_NETWORK_INTERFACE_PCAP_H */
