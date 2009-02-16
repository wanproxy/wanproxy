#ifndef LOG_H
#define LOG_H

#include <sstream>
#include <string>

class LogNull {
public:
	LogNull(void)
	{ }

	~LogNull()
	{ }

	template<typename T>
	LogNull&
	operator << (T x)
	{
		return *this;
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
	Log(const LogHandle& handle, const Priority& priority)
	: handle_(handle),
	  priority_(priority),
	  str_(),
	  pending_(false),
	  halted_(false)
	{
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

	static void log(const Priority&, const LogHandle, const std::string);
};

	/* A panic condition.  */
#define	EMERGENCY(log)	Log(log, Log::Emergency)
	/* A condition that should be corrected immediately.  */
#define	ALERT(log)	Log(log, Log::Alert)
	/* Critical condition.  */
#define	CRITICAL(log)	Log(log, Log::Critical)
	/* Errors.  */
#define	ERROR(log)	Log(log, Log::Error)
	/* Warnings.  */
#define	WARNING(log)	Log(log, Log::Warning)
	/* Conditions that are not error conditions, but may need handled.  */
#define	NOTICE(log)	Log(log, Log::Notice)
	/* Informational.  */
#define	INFO(log)	Log(log, Log::Info)
	/* Debugging information.  */
#if !defined(NDEBUG)
#define	DEBUG(log)	Log(log, Log::Debug)
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
		DEBUG(log_) << "Entered " << function_;
	}

	~Trace()
	{
		DEBUG(log_) << "Exited " << function_;
	}
};

	/* Selective debug tracing.  */
#define	TRACE(log)	Trace trace_(log, __PRETTY_FUNCTION__)

#endif /* !LOG_H */
