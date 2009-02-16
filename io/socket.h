#ifndef	SOCKET_H
#define	SOCKET_H

class Socket : public FileDescriptor {
	LogHandle log_;
	int domain_;
	Action *accept_action_;
	EventCallback *accept_callback_;
	EventCallback *connect_callback_;
	Action *connect_action_;

	Socket(int, int);
public:
	~Socket();

	Action *accept(EventCallback *);
	bool bind(const std::string&, unsigned *);
	Action *connect(const std::string&, unsigned, EventCallback *);
	bool listen(int=10);

private:
	void accept_callback(Event, void *);
	void accept_cancel(void);
	Action *accept_schedule(void);

	void connect_callback(Event, void *);
	void connect_cancel(void);
	Action *connect_schedule(void);

public:
	static Socket *create(int, int, const std::string&);
};

#endif /* !SOCKET_H */
