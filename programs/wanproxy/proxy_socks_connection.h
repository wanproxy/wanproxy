#ifndef	PROXY_SOCKS_CONNECTION_H
#define	PROXY_SOCKS_CONNECTION_H

class ProxySocksConnection {
	enum State {
		GetSOCKSVersion,

		GetSOCKS4Command,
		GetSOCKS4Port,
		GetSOCKS4Address,
		GetSOCKS4User,

		GetSOCKS5AuthLength,
		GetSOCKS5Auth,
		GetSOCKS5Command,
		GetSOCKS5Reserved,
		GetSOCKS5AddressType,
		GetSOCKS5Address,
		GetSOCKS5NameLength,
		GetSOCKS5Name,
		GetSOCKS5Port,
	};

	LogHandle log_;
	Channel *client_;
	Action *action_;
	State state_;
	uint16_t network_port_;
	uint32_t network_address_;
	bool socks5_authenticated_;
	std::string socks5_remote_name_;

public:
	ProxySocksConnection(Channel *);
private:
	~ProxySocksConnection();

private:
	void read_complete(Event, void *);
	void write_complete(Event, void *);
	void close_complete(Event, void *);

	void schedule_read(size_t);
	void schedule_write(void);
	void schedule_close(void);
};

#endif /* !PROXY_SOCKS_CONNECTION_H */
