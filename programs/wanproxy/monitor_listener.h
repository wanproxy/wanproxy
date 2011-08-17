#ifndef	MONITOR_LISTENER_H
#define	MONITOR_LISTENER_H

class Socket;
class TCPServer;
class XCodec;

class MonitorListener {
	LogHandle log_;
	std::string name_;
	TCPServer *server_;
	Action *accept_action_;
	Action *close_action_;
	Action *stop_action_;
	std::string interface_;

public:
	MonitorListener(const std::string&, SocketAddressFamily, const std::string&);
	~MonitorListener();

private:
	void accept_complete(Event, Socket *);
	void close_complete(void);
	void stop(void);
};

#endif /* !MONITOR_LISTENER_H */
