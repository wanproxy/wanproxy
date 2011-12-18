#include <sys/time.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#ifdef	USE_SYSLOG
#include <syslog.h>
#endif

#include <iostream>
#include <list>
#include <sstream>

struct LogMask {
	regex_t regex_;
	enum Log::Priority priority_;
};

static std::list<LogMask> log_masks;

#ifdef	USE_SYSLOG
static int syslog_priority(const Log::Priority&);
#endif
static std::ostream& operator<< (std::ostream&, const Log::Priority&);
static std::ostream& operator<< (std::ostream&, const struct timeval&);

void
Log::log(const Priority& priority, const LogHandle& handle,
	 const std::string& message)
{
	std::list<LogMask>::const_iterator it;
	std::string handle_string = (std::string)handle;

	/*
	 * XXX
	 * Skip this all if we're in a HALT or NOTREACHED, since we can
	 * generate those here.  Critical logs cannot ever be masked, right?
	 */
	for (it = log_masks.begin(); it != log_masks.end(); ++it) {
		const LogMask& mask = *it;
		int rv;

		rv = regexec(&mask.regex_, handle_string.c_str(), 0, NULL, 0);
		switch (rv) {
		case 0:
			if (priority <= mask.priority_)
				goto done;
			return;
		case REG_NOMATCH:
			continue;
		default:
			HALT("/log") << "Could not match regex: " << rv;
			return;
		}
		NOTREACHED("/log");
	}

done:
#ifdef	USE_SYSLOG
	std::string syslog_message;

	syslog_message += "[";
	syslog_message += (std::string)handle;
	syslog_message += "] ";
	syslog_message += message;

	syslog(syslog_priority(priority), "%s", syslog_message.c_str());
#endif

	struct timeval now;
	int rv;

	rv = gettimeofday(&now, NULL);
	if (rv == -1)
		memset(&now, 0, sizeof now);

	std::cerr << now << " [" << handle_string << "] " <<
		priority << ": " <<
		message <<
		std::endl;
}

bool
Log::mask(const std::string& handle_regex, const Log::Priority& priority)
{
	LogMask mask;

	if (::regcomp(&mask.regex_, handle_regex.c_str(),
		      REG_NOSUB | REG_EXTENDED) != 0) {
		return (false);
	}
	mask.priority_ = priority;

	log_masks.push_back(mask);

	return (true);
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
	case Log::Trace:
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
	case Log::Trace:
		return (os << "TRACE");
	default:
		HALT("/log") << "Unhandled log priority!";
		return (os);
	}
}

static std::ostream&
operator<< (std::ostream& os, const struct timeval& tv)
{
	char buf[20];

	snprintf(buf, sizeof buf, "%u.%06u", (unsigned)tv.tv_sec,
		 (unsigned)tv.tv_usec);
	return (os << buf);
}
