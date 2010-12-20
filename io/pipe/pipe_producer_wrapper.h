#ifndef	IO_PIPE_PRODUCER_WRAPPER_H
#define	IO_PIPE_PRODUCER_WRAPPER_H

template<typename T>
class PipeProducerWrapper : public PipeProducer {
	typedef void (T::*consume_method_t)(Buffer *);

	T *obj_;
	consume_method_t consume_method_;
public:
	template<typename Tp>
	PipeProducerWrapper(T *obj, Tp consume_method)
	: PipeProducer("/pipe/producer/wrapper"),
	  obj_(obj),
	  consume_method_(consume_method)
	{ }

	~PipeProducerWrapper()
	{ }

	void consume(Buffer *buf)
	{
		return ((obj_->*consume_method_)(buf));
	}
};

#endif /* !IO_PIPE_PRODUCER_WRAPPER_H */
