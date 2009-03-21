#include <deque>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/file.h>

#include "flow_monitor.h"
#include "flow_table.h"
#include "proxy_listener.h"
#include "proxy_socks_listener.h"
#include "wanproxy_config.h"

WANProxyConfig::WANProxyConfig(void)
: log_("/wanproxy/config"),
  codec_(NULL),
  flow_monitor_(NULL),
  flow_tables_(),
  proxy_listeners_(),
  proxy_socks_listeners_(),
  config_file_(NULL),
  close_action_(NULL),
  read_action_(NULL),
  read_buffer_()
{ }

WANProxyConfig::~WANProxyConfig()
{
	ASSERT(config_file_ == NULL);
	ASSERT(close_action_ == NULL);
	ASSERT(read_action_ == NULL);
	ASSERT(read_buffer_.empty());

	if (flow_monitor_ != NULL) {
		delete flow_monitor_;
		flow_monitor_ = NULL;
	}

	/* XXX Delete listeners, etc.  */
}

void
WANProxyConfig::close_complete(Event e)
{
	close_action_->cancel();
	close_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		HALT(log_) << "Unexpected event: " << e;
	}

	ASSERT(config_file_ != NULL);
	delete config_file_;
	config_file_ = NULL;
}

void
WANProxyConfig::read_complete(Event e)
{
	read_action_->cancel();
	read_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::EOS:
		break;
	default:
		HALT(log_) << "Unexpected event: " << e;
	}

	read_buffer_.append(e.buffer_);

	parse();

#if 0
	if (e.type_ == Event::EOS)
		schedule_close();
	else
		schedule_read();
#else
	/*
	 * XXX
	 * The FileDescriptor class doesn't work right with Files yet.  Namely,
	 * it won't detect EOF.  I think the reason is that the EVFILT_READ
	 * filter for kqueue doesn't fire if you're at EOF, which seems dumb.
	 * Need to investigate further.  For now, the config files are small
	 * enough that this works.
	 */
	schedule_close();
#endif
}

void
WANProxyConfig::schedule_close(void)
{
	ASSERT(close_action_ == NULL);
	ASSERT(config_file_ != NULL);
	EventCallback *cb = callback(this, &WANProxyConfig::close_complete);
	close_action_ = config_file_->close(cb);
}

void
WANProxyConfig::schedule_read(void)
{
	ASSERT(read_action_ == NULL);
	ASSERT(config_file_ != NULL);
	EventCallback *cb = callback(this, &WANProxyConfig::read_complete);
	read_action_ = config_file_->read(0, cb);
}

void
WANProxyConfig::parse(void)
{
	unsigned pos;
	while (read_buffer_.find('\n', &pos)) {
		if (read_buffer_.peek() == '#' || pos == 0) {
			read_buffer_.skip(pos + 1);
			continue;
		}

		Buffer line(read_buffer_, pos);
		read_buffer_.skip(pos + 1);

		std::deque<std::string> tokens;
		while (!line.empty()) {
			unsigned wordlen;
			if (line.find(' ', &pos)) {
				wordlen = pos; 
			} else {
				wordlen = line.length();
			}
			uint8_t wordbytes[wordlen];
			line.moveout(wordbytes, wordlen);
			if (!line.empty()) {
				ASSERT(line.peek() == ' ');
				line.skip(1);
			}
			std::string word((const char *)wordbytes, wordlen);
			tokens.push_back(word);
		}

		parse(tokens);
		ASSERT(tokens.empty());
	}
}

void
WANProxyConfig::parse(std::deque<std::string>& tokens)
{
	std::string command = tokens.front();
	tokens.pop_front();

	if (command == "flow-monitor") {
		parse_flow_monitor(tokens);
	} else if (command == "flow-table") {
		parse_flow_table(tokens);
	} else if (command == "log-mask") {
		parse_log_mask(tokens);
	} else if (command == "proxy") {
		parse_proxy(tokens);
	} else if (command == "proxy-socks") {
		parse_proxy_socks(tokens);
	} else {
		tokens.clear();
		ERROR(log_) << "Unrecognized configuration command: " << command;
	}
	if (!tokens.empty())
		ERROR(log_) << "Unconsumed tokens.";
	tokens.clear();
}

void
WANProxyConfig::parse_flow_monitor(std::deque<std::string>& tokens)
{
	if (tokens.size() != 0) {
		ERROR(log_) << "Wrong number of words in flow-monitor (" << tokens.size() << ")";
		return;
	}

	if (flow_monitor_ != NULL) {
		ERROR(log_) << "Ignoring duplicate flow-monitor.";
		return;
	}
	flow_monitor_ = new FlowMonitor();
}

void
WANProxyConfig::parse_flow_table(std::deque<std::string>& tokens)
{
	if (tokens.size() != 1) {
		ERROR(log_) << "Wrong number of words in flow-table (" << tokens.size() << ")";
		return;
	}

	std::string table = tokens.front();
	tokens.pop_front();

	if (flow_tables_.find(table) != flow_tables_.end()) {
		ERROR(log_) << "Duplicate definition of flow-table " << table;
		return;
	}

	FlowTable *flow_table = new FlowTable(table);

	flow_tables_[table] = flow_table;

	if (flow_monitor_ == NULL) {
		INFO(log_) << "Not monitoring flow-table " << table;
	} else {
		flow_monitor_->monitor(table, flow_table);
	}
}

void
WANProxyConfig::parse_log_mask(std::deque<std::string>& tokens)
{
	if (tokens.size() != 2) {
		ERROR(log_) << "Wrong number of words in log-mask (" << tokens.size() << ")";
		return;
	}

	std::string handle_regex = tokens.front();
	tokens.pop_front();

	std::string priority_mask = tokens.front();
	tokens.pop_front();

	if (!Log::mask(handle_regex, priority_mask)) {
		ERROR(log_) << "Unable to set log handle \"" << handle_regex << "\" mask to priority \"" << priority_mask << "\"";
	}
}

void
WANProxyConfig::parse_proxy(std::deque<std::string>& tokens)
{
	if (tokens.size() != 11) {
		ERROR(log_) << "Wrong number of words in proxy (" << tokens.size() << ")";
		return;
	}

	if (tokens.front() != "flow-table") {
		ERROR(log_) << "Missing 'flow-table' statement.";
		return;
	}
	tokens.pop_front();

	FlowTable *flow_table = flow_tables_[tokens.front()];
	if (flow_table == NULL) {
		ERROR(log_) << "Flow table '" << tokens.front() << "' not present.";
		return;
	}
	tokens.pop_front();

	std::string local_host = tokens.front();
	tokens.pop_front();
	unsigned local_port = atoi(tokens.front().c_str());
	tokens.pop_front();

	if (tokens.front() != "decoder") {
		ERROR(log_) << "Missing 'decoder' statement.";
		return;
	}
	tokens.pop_front();

	XCodec *local_codec;
	if (tokens.front() == "xcodec")
		local_codec = codec_;
	else if (tokens.front() == "none")
		local_codec = NULL;
	else {
		ERROR(log_) << "Malformed decoder name.";
		return;
	}
	tokens.pop_front();

	if (tokens.front() != "to") {
		ERROR(log_) << "Missing 'to' statement.";
		return;
	}
	tokens.pop_front();

	std::string remote_host = tokens.front();
	tokens.pop_front();
	unsigned remote_port = atoi(tokens.front().c_str());
	tokens.pop_front();

	if (tokens.front() != "encoder") {
		ERROR(log_) << "Missing 'encoder' statement.";
		return;
	}
	tokens.pop_front();

	XCodec *remote_codec;
	if (tokens.front() == "xcodec")
		remote_codec = codec_;
	else if (tokens.front() == "none")
		remote_codec = NULL;
	else {
		ERROR(log_) << "Malformed encoder name.";
		return;
	}
	tokens.pop_front();

	INFO(log_) << "Starting proxy from " << local_host << ':' << local_port << " to " << remote_host << ':' << remote_port;

	ProxyListener *listener = new ProxyListener(flow_table,
						    local_codec,
						    remote_codec,
						    local_host,
						    local_port,
						    remote_host,
						    remote_port);
	proxy_listeners_.insert(listener);
}

void
WANProxyConfig::parse_proxy_socks(std::deque<std::string>& tokens)
{
	if (tokens.size() != 4) {
		ERROR(log_) << "Wrong number of words in proxy-socks (" << tokens.size() << ")";
		return;
	}

	if (tokens.front() != "flow-table") {
		ERROR(log_) << "Missing 'flow-table' statement.";
		return;
	}
	tokens.pop_front();

	FlowTable *flow_table = flow_tables_[tokens.front()];
	if (flow_table == NULL) {
		ERROR(log_) << "Flow table '" << tokens.front() << "' not present.";
		return;
	}
	tokens.pop_front();

	std::string local_host = tokens.front();
	tokens.pop_front();
	unsigned local_port = atoi(tokens.front().c_str());
	tokens.pop_front();

	INFO(log_) << "Starting socks-proxy on " << local_host << ':' << local_port;

	ProxySocksListener *listener = new ProxySocksListener(flow_table,
							      local_host,
							      local_port);
	proxy_socks_listeners_.insert(listener);
}

bool
WANProxyConfig::configure(XCodec *codec, const std::string& name)
{
	INFO(log_) << "Configuring WANProxy.";

	if (config_file_ != NULL || codec_ != NULL) {
		ERROR(log_) << "WANProxy already configured.";
		return (false);
	}

	FileDescriptor *file = File::open(name, true, false);
	if (file == NULL) {
		ERROR(log_) << "Could not open file: " << name;
		return (false);
	}

	codec_ = codec;
	config_file_ = file;

	schedule_read();

	return (true);
}
