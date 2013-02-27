/*
 * Copyright (c) 2008-2011 Juli Mallett. All rights reserved.
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

#ifndef	COMMON_LOG_H
#define	COMMON_LOG_H

#include <stdlib.h> /* For abort(3).  */

#include <sstream>
#include <string>

class LogNull {
public:
	LogNull(void)
	{ }

	~LogNull()
	{ }

	template<typename T>
	const LogNull& operator<< (T) const
	{
		return (*this);
	}

	const LogNull& operator<< (std::ostream& (*)(std::ostream&)) const
	{
		return (*this);
	}
};

class LogHandle {
	std::string string_;
public:
	LogHandle(const char *s)
	: string_(s)
	{ }

	LogHandle(const std::string& s)
	: string_(s)
	{ }

	~LogHandle()
	{ }

	operator const std::string& (void) const
	{
		return (string_);
	}

	template<typename T>
	LogHandle operator+ (T x) const
	{
		LogHandle appended = *this;
		appended.string_ += x;
		return (appended);
	}
};

class Log {
public:
	enum Priority {
		Emergency,
		Alert,
		Critical,
		Error,
		Warning,
		Notice,
		Info,
		Trace,
		Debug,
	};

	class Halt {
	public:
		Halt(void)
		{ }

		~Halt()
		{ }
	};

private:
	LogHandle handle_;
	Priority priority_;
	std::ostringstream str_;
	bool pending_;
	bool halted_;
public:
	Log(const LogHandle& handle, const Priority& priority, const std::string function)
	: handle_(handle),
	  priority_(priority),
	  str_(),
	  pending_(false),
	  halted_(false)
	{
		/*
		 * Print the function name for non-routine messages.
		 */
		switch (priority_) {
		case Emergency:
		case Alert:
		case Critical:
		case Error:
#if !defined(NDEBUG)
		case Debug:
#endif
			str_ << function << ": ";
			break;
		default:
			break;
		}
	}

	~Log()
	{
		flush();
		if (halted_) {
			abort(); /* XXX */
		}
	}

	template<typename T>
	std::ostream&
	operator << (T x)
	{
		pending_ = true;
		return (str_ << x);
	}

	std::ostream&
	operator << (const Halt&)
	{
		pending_ = true;
		halted_ = true;
		return (str_ << "Halting: ");
	}

	void
	flush(void)
	{
		if (!pending_)
			return;
		Log::log(priority_, handle_, str_.str());
	}

	static void log(const Priority&, const LogHandle&, const std::string&);
	static bool mask(const std::string&, const Priority&);
};

	/* A panic condition.  */
#define	EMERGENCY(log)	Log(log, Log::Emergency, __PRETTY_FUNCTION__)
	/* A condition that should be corrected immediately.  */
#define	ALERT(log)	Log(log, Log::Alert, __PRETTY_FUNCTION__)
	/* Critical condition.  */
#define	CRITICAL(log)	Log(log, Log::Critical, __PRETTY_FUNCTION__)
	/* Errors.  */
#define	ERROR(log)	Log(log, Log::Error, __PRETTY_FUNCTION__)
	/* Warnings.  */
#define	WARNING(log)	Log(log, Log::Warning, __PRETTY_FUNCTION__)
	/* Conditions that are not error conditions, but may need handled.  */
#define	NOTICE(log)	Log(log, Log::Notice, __PRETTY_FUNCTION__)
	/* Informational.  */
#define	INFO(log)	Log(log, Log::Info, __PRETTY_FUNCTION__)
	/* Debugging information.  */
#if !defined(NDEBUG)
#define	DEBUG(log)	Log(log, Log::Debug, __PRETTY_FUNCTION__)
#else
#define	DEBUG(log)	LogNull()
#endif

	/* A condition which cannot be continued from.  */
#define	HALT(log)	EMERGENCY(log) << Log::Halt()

class Trace {
	LogHandle log_;
	std::string function_;
public:
	Trace(const LogHandle& log, const std::string& function)
	: log_(log),
	  function_(function)
	{
		Log(log_, Log::Trace, __PRETTY_FUNCTION__) << "Entered " << function_;
	}

	~Trace()
	{
		Log(log_, Log::Trace, __PRETTY_FUNCTION__) << "Exited " << function_;
	}
};

	/* Selective debug tracing.  */
#define	TRACE(log)	Trace trace_(log, __PRETTY_FUNCTION__)

#endif /* !COMMON_LOG_H */
