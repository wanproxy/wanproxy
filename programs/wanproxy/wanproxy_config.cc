#include <deque>
#include <fstream>
#include <sstream>

#include <common/buffer.h>

#include <config/config.h>
#include <config/config_class.h>
#include <config/config_class_log_mask.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include "wanproxy_config.h"
#include "wanproxy_config_class_codec.h"
#include "wanproxy_config_class_interface.h"
#include "wanproxy_config_class_peer.h"
#include "wanproxy_config_class_proxy.h"
#include "wanproxy_config_class_proxy_socks.h"

WANProxyConfig::WANProxyConfig(void)
: log_("/wanproxy/config"),
  config_(NULL)
{ }

WANProxyConfig::~WANProxyConfig()
{
	if (config_ != NULL) {
		delete config_;
		config_ = NULL;
	}
}

bool
WANProxyConfig::parse(std::deque<std::string>& tokens)
{
	std::string command = tokens.front();
	tokens.pop_front();

	if (command == "activate") {
		parse_activate(tokens);
	} else if (command == "create") {
		parse_create(tokens);
	} else if (command == "set") {
		parse_set(tokens);
	} else {
		tokens.clear();
		ERROR(log_) << "Unrecognized configuration command: " << command;
	}
	if (!tokens.empty())
		return (false);
	return (true);
}

void
WANProxyConfig::parse_activate(std::deque<std::string>& tokens)
{
	if (tokens.size() != 1) {
		ERROR(log_) << "Wrong number of words in activate (" << tokens.size() << ")";
		return;
	}

	if (!config_->activate(tokens[0])) {
		ERROR(log_) << "Object (" << tokens[0] << ") activation failed.";
		return;
	}
	tokens.clear();
}

void
WANProxyConfig::parse_create(std::deque<std::string>& tokens)
{
	if (tokens.size() != 2) {
		ERROR(log_) << "Wrong number of words in create (" << tokens.size() << ")";
		return;
	}

	if (!config_->create(tokens[0], tokens[1])) {
		ERROR(log_) << "Object (" << tokens[1] << ") could not be created.";
		return;
	}
	tokens.clear();
}

void
WANProxyConfig::parse_set(std::deque<std::string>& tokens)
{
	if (tokens.size() != 2) {
		ERROR(log_) << "Wrong number of words in set (" << tokens.size() << ")";
		return;
	}

	std::string::iterator dot = std::find(tokens[0].begin(), tokens[0].end(), '.');
	if (dot == tokens[0].end()) {
		ERROR(log_) << "Identifier (" << tokens[0] << ") is not an object member identifier.";
		return;
	}

	std::string object(tokens[0].begin(), dot);
	std::string member(dot + 1, tokens[0].end());
	if (object == "" || member == "") {
		ERROR(log_) << "Object (" << object << ") or member name (" << member << ") is empty.";
		return;
	}

	if (!config_->set(object, member, tokens[1])) {
		ERROR(log_) << "Set of object member (" << tokens[0] << ") failed.";
		return;
	}
	tokens.clear();
}

bool
WANProxyConfig::configure(const std::string& name)
{
	if (config_ != NULL) {
		ERROR(log_) << "WANProxy already configured.";
		return (false);
	}

	INFO(log_) << "Configuring WANProxy.";

	std::fstream in;
	in.open(name.c_str(), std::ios::in);

	if (!in.good()) {
		ERROR(log_) << "Could not open file: " << name;
		return (false);
	}

	config_ = new Config();
	config_->import(&config_class_log_mask);
	config_->import(&wanproxy_config_class_interface);
	config_->import(&wanproxy_config_class_peer);
	config_->import(&wanproxy_config_class_codec);
	config_->import(&wanproxy_config_class_proxy);
	config_->import(&wanproxy_config_class_proxy_socks);

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
			tokens.push_back(word);
		}
		ASSERT(!tokens.empty());
		if (!parse(tokens)) {
			ERROR(log_) << "Error in configuration directive: " << line;

			delete config_;
			config_ = NULL;

			return (false);
		}
		ASSERT(tokens.empty());
	}

	return (true);
}
