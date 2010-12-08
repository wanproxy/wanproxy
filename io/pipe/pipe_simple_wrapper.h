#ifndef	IO_PIPE_SIMPLE_WRAPPER_H
#define	IO_PIPE_SIMPLE_WRAPPER_H

template<typename T>
class PipeSimpleWrapper : public PipeSimple {
	typedef bool (T::*process_method_t)(Buffer *, Buffer *);

	T *obj_;
	process_method_t process_method_;
public:
	template<typename Tp>
	PipeSimpleWrapper(T *obj, Tp process_method)
	: PipeSimple("/pipe/simple/wrapper"),
	  obj_(obj),
	  process_method_(process_method)
	{ }

	~PipeSimpleWrapper()
	{ }

	bool process(Buffer *out, Buffer *in)
	{
		return ((obj_->*process_method_)(out, in));
	}
};

#endif /* !IO_PIPE_SIMPLE_WRAPPER_H */
