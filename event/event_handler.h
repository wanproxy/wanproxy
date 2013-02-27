/*
 * Copyright (c) 2012 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	EVENT_EVENT_HANDLER_H
#define	EVENT_EVENT_HANDLER_H

#include <map>

#include <event/event.h>
#include <event/event_callback.h>

class EventHandler {
	typedef	std::map<Event::Type, EventCallback *> callback_map_t;

	Action *action_;
	callback_map_t callback_map_;
	EventCallback *default_;
public:
	EventHandler(void)
	: action_(NULL),
	  callback_map_(),
	  default_(NULL)
	{ }

	~EventHandler()
	{
		if (action_ != NULL) {
			action_->cancel();
			action_ = NULL;
		}

		if (!callback_map_.empty()) {
			callback_map_t::iterator it;
			for (it = callback_map_.begin(); it != callback_map_.end(); ++it)
				delete it->second;
			callback_map_.clear();
		}

		if (default_ != NULL) {
			delete default_;
			default_ = NULL;
		}
	}

	EventCallback *callback(void)
	{
		ASSERT("/event/handler", !callback_map_.empty() || default_ != NULL);
		return (::callback(this, &EventHandler::handle_callback));
	}

	void cancel(void)
	{
		ASSERT("/event/handler", action_ != NULL);

		action_->cancel();
		action_ = NULL;
	}

	template<typename Tobj, typename Tmethod>
	void handler(Tobj obj, Tmethod method)
	{
		ASSERT("/event/handler", action_ == NULL);

		if (default_ != NULL)
			HALT("/event/handler") << "Duplicate default handler installed.";
		default_ = ::callback(obj, method);
	}

	template<typename Tobj, typename Tmethod, typename Targ>
	void handler(Tobj obj, Tmethod method, Targ arg)
	{
		ASSERT("/event/handler", action_ == NULL);

		if (default_ != NULL)
			HALT("/event/handler") << "Duplicate default handler installed.";
		default_ = ::callback(obj, method, arg);
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
		if (it != callback_map_.end() || default_ != NULL) {
			EventCallback *cb;

			if (it != callback_map_.end())
				cb = it->second;
			else
				cb = default_;

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
