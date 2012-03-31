#include <deque>
#include <fstream>
#include <sstream>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/socket/socket.h>

#include <io/net/tcp_server.h>

#include "fwdproxy_config.h"

#include "proxy_listener.h"

FWDProxyConfig::FWDProxyConfig(void)
: log_("/fwdproxy/config")
{ }

FWDProxyConfig::~FWDProxyConfig()
{ }

bool
FWDProxyConfig::parse(std::deque<std::string>& tokens)
{
	if (tokens.size() != 4) {
		ERROR(log_) << "Wrong number of items in configuration entry.  Expected 4, got " << tokens.size() << ".";
		return (false);
	}

	std::string interface = "[" + tokens[0] + "]:" + tokens[1];
	std::string remote = "[" + tokens[2] + "]:" + tokens[3];

	tokens.clear();

	new ProxyListener("configured", SocketAddressFamilyIP, interface, SocketAddressFamilyIP, remote);
	return (true);
}

bool
FWDProxyConfig::configure(const std::string& name)
{
	INFO(log_) << "Configuring FWDProxy.";

	std::fstream in;
	in.open(name.c_str(), std::ios::in);

	if (!in.good()) {
		ERROR(log_) << "Could not open file: " << name;
		return (false);
	}

	std::deque<std::string> tokens;

	while (in.good()) {
		std::string line;
		std::getline(in, line);

		if (line[0] == '#' || line.empty())
			continue;

		std::istringstream is(line);
		while (is.good()) {
			std::string word;
			is >> word;
			if (word.empty())
				continue;
			if (word[0] == '#')
				break;
			tokens.push_back(word);
		}
		ASSERT(log_, !tokens.empty());
		if (!parse(tokens)) {
			ERROR(log_) << "Error in configuration directive: " << line;

			return (false);
		}
		ASSERT(log_, tokens.empty());
	}

	return (true);
}
