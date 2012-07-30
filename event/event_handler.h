#ifndef	EVENT_EVENT_HANDLER_H
#define	EVENT_EVENT_HANDLER_H

#include <map>

#include <event/event.h>
#include <event/event_callback.h>

class EventHandler {
	typedef	std::map<Event::Type, EventCallback *> callback_map_t;

	Action *action_;
	callback_map_t callback_map_;
public:
	EventHandler(void)
	: action_(NULL),
	  callback_map_()
	{ }

	~EventHandler()
	{
		if (action_ != NULL) {
			action_->cancel();
			action_ = NULL;
		}

		ASSERT("/event/handler", !callback_map_.empty());
		callback_map_t::iterator it;
		for (it = callback_map_.begin(); it != callback_map_.end(); ++it)
			delete it->second;
		callback_map_.clear();
	}

	EventCallback *callback(void)
	{
		ASSERT("/event/handler", !callback_map_.empty());
		return (::callback(this, &EventHandler::handle_callback));
	}

	void cancel(void)
	{
		ASSERT("/event/handler", action_ != NULL);

		action_->cancel();
		action_ = NULL;
	}

	template<typename Tobj, typename Tmethod>
	void handler(Event::Type param, Tobj obj, Tmethod method)
	{
		ASSERT("/event/handler", action_ == NULL);

		callback_map_t::const_iterator it = callback_map_.find(param);
		if (it != callback_map_.end())
			HALT("/event/handler") << "Duplicate handler installed for: " << param;
		callback_map_[param] = ::callback(obj, method);
	}

	template<typename Tobj, typename Tmethod, typename Targ>
	void handler(Event::Type param, Tobj obj, Tmethod method, Targ arg)
	{
		ASSERT("/event/handler", action_ == NULL);

		callback_map_t::const_iterator it = callback_map_.find(param);
		if (it != callback_map_.end())
			HALT("/event/handler") << "Duplicate handler installed for: " << param;
		callback_map_[param] = ::callback(obj, method, arg);
	}

	void wait(Action *action)
	{
		ASSERT("/event/handler", action_ == NULL);
		ASSERT("/event/handler", action != NULL);

		action_ = action;
	}

private:
	void handle_callback(Event e)
	{
		ASSERT("/event/handler", action_ != NULL);

		action_->cancel();
		action_ = NULL;

		callback_map_t::const_iterator it = callback_map_.find(e.type_);
		if (it != callback_map_.end()) {
			EventCallback *cb = it->second;

			cb->param(e);
			cb->execute();
			cb->reset();
			return;
		}

		HALT("/event/handler") << "Unhandled event: " << e;

		/*
		 * XXX
		 * This is a great place to hook in for debugging/tracing
		 * and to show whether another action was queued inside the
		 * callback execution path, etc.
		 */
	}
};

#endif /* !EVENT_EVENT_HANDLER_H */
