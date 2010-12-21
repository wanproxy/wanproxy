#ifndef	IO_PIPE_SIMPLE_WRAPPER_H
#define	IO_PIPE_SIMPLE_WRAPPER_H

#include <io/pipe/pipe_producer.h>

template<typename T>
class PipeSimpleWrapper : public PipeProducer {
	typedef bool (T::*process_method_t)(Buffer *, Buffer *);

	T *obj_;
	process_method_t process_method_;
public:
	template<typename Tp>
	PipeSimpleWrapper(T *obj, Tp process_method)
	: PipeProducer("/pipe/simple/wrapper"),
	  obj_(obj),
	  process_method_(process_method)
	{ }

	~PipeSimpleWrapper()
	{ }

	void consume(Buffer *in)
	{
		Buffer out;
		if (!(obj_->*process_method_)(&out, in)) {
			produce_error();
			return;
		}
		produce(&out);
	}
};

#endif /* !IO_PIPE_SIMPLE_WRAPPER_H */
