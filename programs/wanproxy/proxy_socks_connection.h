#ifndef	PROXY_SOCKS_CONNECTION_H
#define	PROXY_SOCKS_CONNECTION_H

class ProxySocksConnection {
	enum State {
		GetVersion,
		GetCommand,
		GetPort,
		GetAddress,
		GetUser,
	};

	LogHandle log_;
	Channel *client_;
	Action *action_;
	State state_;
	uint16_t network_port_;
	uint32_t network_address_;

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
