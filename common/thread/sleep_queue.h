#ifndef	COMMON_THREAD_SLEEP_QUEUE_H
#define	COMMON_THREAD_SLEEP_QUEUE_H

/*
 * We don't call this a condition variable because that name is in use in the
 * event system, which also sees more general use.  Since this is only used for
 * limited internal facilities, the more obscure name is acceptable, but it's
 * basically a condition variable.
 */

struct SleepQueueState;

class SleepQueue {
	std::string name_;
	Mutex *mutex_;
	SleepQueueState *state_;
public:
	SleepQueue(const std::string&, Mutex *);
	~SleepQueue();

	void signal(void);
	void wait(void);
};

#endif /* !COMMON_THREAD_SLEEP_QUEUE_H */
