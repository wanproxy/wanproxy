#ifndef	IO_SYSTEM_H
#define	IO_SYSTEM_H

#include <io/io_system.h>

class Channel;

class IOSystem {
	struct Handle {
		LogHandle log_;

		int fd_;
		Channel *owner_;

		EventCallback *close_callback_;
		Action *close_action_;

		size_t read_amount_;
		Buffer read_buffer_;
		EventCallback *read_callback_;
		Action *read_action_;

		Buffer write_buffer_;
		EventCallback *write_callback_;
		Action *write_action_;

		Handle(int, Channel *);
		~Handle();

		void close_callback(void);
		void close_cancel(void);
		Action *close_schedule(void);

		void read_callback(Event, void *);
		void read_cancel(void);
		Action *read_schedule(void);

		void write_callback(Event, void *);
		void write_cancel(void);
		Action *write_schedule(void);
	};

	LogHandle log_;
	std::map<int, Handle *> handle_map_;

	IOSystem(void);
	~IOSystem();

public:
	void attach(int, Channel *);
	void detach(int, Channel *);

	Action *close(int, EventCallback *);
	Action *read(int, size_t, EventCallback *);
	Action *write(int, Buffer *, EventCallback *);

	static IOSystem *instance(void)
	{
		static IOSystem *instance_;

		if (instance_ == NULL)
			instance_ = new IOSystem();
		return (instance_);
	}
};

#endif /* !IO_SYSTEM_H */
