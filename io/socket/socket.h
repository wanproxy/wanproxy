#ifndef	SOCKET_H
#define	SOCKET_H

#include <event/event.h>
#include <event/object_callback.h>
#include <event/typed_pair_callback.h>

#include <io/stream_handle.h>
#include <io/socket/socket_types.h>

typedef	class TypedPairCallback<Event, Socket *> SocketEventCallback;

class Socket : public StreamHandle {
	LogHandle log_;
	int domain_;
	int socktype_;
	int protocol_;
	Action *accept_action_;
	SocketEventCallback *accept_callback_;
	EventCallback *connect_callback_;
	Action *connect_action_;

	Socket(int, int, int, int);
public:
	~Socket();

	virtual Action *accept(SocketEventCallback *);
	bool bind(const std::string&);
	Action *connect(const std::string&, EventCallback *);
	bool listen(int=10);
	Action *shutdown(bool, bool, EventCallback *);

	std::string getpeername(void) const;
	std::string getsockname(void) const;

private:
	void accept_callback(Event);
	void accept_cancel(void);
	Action *accept_schedule(void);

	void connect_callback(Event);
	void connect_cancel(void);
	Action *connect_schedule(void);

public:
	static Socket *create(SocketAddressFamily, SocketType, const std::string& = "", const std::string& = "");
};

#endif /* !SOCKET_H */
