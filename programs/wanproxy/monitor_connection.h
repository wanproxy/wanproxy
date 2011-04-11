#ifndef	MONITOR_CONNECTION_H
#define	MONITOR_CONNECTION_H

class MonitorConnection {
	LogHandle log_;
	std::string name_;
	Socket *client_;
	Action *action_;

public:
	MonitorConnection(const std::string&, Socket *);
private:
	~MonitorConnection();

	void write_complete(Event);
	void close_complete(void);

	void schedule_close(void);
};

#endif /* !MONITOR_CONNECTION_H */
