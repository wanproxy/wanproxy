/*
 * Copyright (c) 2012-2013 Juli Mallett. All rights reserved.
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
