#ifndef	NETWORK_NETWORK_INTERFACE_H
#define	NETWORK_NETWORK_INTERFACE_H

class NetworkInterface {
protected:
	NetworkInterface(void)
	{ }

public:
	virtual ~NetworkInterface()
	{ }

	virtual Action *receive(EventCallback *) = 0;
	virtual Action *transmit(Buffer *, EventCallback *) = 0;

	static NetworkInterface *open(const std::string&);
};

#endif /* !NETWORK_NETWORK_INTERFACE_H */
