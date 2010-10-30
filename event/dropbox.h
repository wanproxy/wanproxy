#ifndef	DROPBOX_H
#define	DROPBOX_H

#include <event/typed_callback.h>
#include <event/typed_condition.h>

template<typename T>
class Dropbox {
	T item_;
	bool item_dropped_;
	TypedConditionVariable<T> item_condition_;
public:
	Dropbox(void)
	: item_(),
	  item_dropped_(false),
	  item_condition_()
	{ }

	~Dropbox()
	{ }

	Action *get(TypedCallback<T> *cb)
	{
		Action *a = item_condition_.wait(cb);
		if (item_dropped_)
			item_condition_.signal(item_);
		return (a);
	}

	void put(T item)
	{
		if (item_dropped_)
			DEBUG("/dropbox") << "Dropbox contents replaced.";
		item_ = item;
		item_dropped_ = true;
		item_condition_.signal(item_);
	}
};

#endif /* !DROPBOX_H */
