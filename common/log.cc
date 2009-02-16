#ifdef	USE_SYSLOG
#include <syslog.h>
#endif

#include <iostream>
#include <sstream>
#include <string>

#ifdef	USE_SYSLOG
static int syslog_priority(const Log::Priority&);
#endif
static std::ostream& operator<< (std::ostream&, const Log::Priority&);

void
Log::log(const Priority& priority, const LogHandle handle,
	 const std::string message)
{
#ifdef	USE_SYSLOG
	std::string syslog_message;

	syslog_message += "[";
	syslog_message += (std::string)handle;
	syslog_message += "]";
	syslog_message += " ";
	syslog_message += message;

	syslog(syslog_priority(priority), "%s", syslog_message.c_str());
#endif

	std::cerr << "Log(" << (std::string)handle << ") " <<
		priority << ": " <<
		message <<
		std::endl;
}

#ifdef	USE_SYSLOG
static int
syslog_priority(const Log::Priority& priority)
{
	switch (priority) {
	case Log::Emergency:
		return (LOG_EMERG);
	case Log::Alert:
		return (LOG_ALERT);
	case Log::Critical:
		return (LOG_CRIT);
	case Log::Error:
		return (LOG_ERR);
	case Log::Warning:
		return (LOG_WARNING);
	case Log::Notice:
		return (LOG_NOTICE);
	case Log::Info:
		return (LOG_INFO);
	case Log::Debug:
		return (LOG_DEBUG);
	default:
		HALT("/log") << "Unhandled log priority!";
		return (-1);
	}
}
#endif

static std::ostream&
operator<< (std::ostream& os, const Log::Priority& priority)
{
	switch (priority) {
	case Log::Emergency:
		return (os << "EMERG");
	case Log::Alert:
		return (os << "ALERT");
	case Log::Critical:
		return (os << "CRIT");
	case Log::Error:
		return (os << "ERR");
	case Log::Warning:
		return (os << "WARNING");
	case Log::Notice:
		return (os << "NOTICE");
	case Log::Info:
		return (os << "INFO");
	case Log::Debug:
		return (os << "DEBUG");
	default:
		HALT("/log") << "Unhandled log priority!";
		return (os);
	}
}
