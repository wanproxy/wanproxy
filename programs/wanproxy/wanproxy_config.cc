#include <vector>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/file.h>

#include "proxy_listener.h"
#include "proxy_socks_listener.h"
#include "wanproxy_config.h"

WANProxyConfig::WANProxyConfig(void)
: log_("/proxy/config"),
  codec_(NULL),
  listeners_(),
  socks_listeners_(),
  config_file_(NULL),
  close_action_(NULL),
  read_action_(NULL),
  read_buffer_()
{
	DEBUG(log_) << "Creating WANProxyConfig instance.";
}

WANProxyConfig::~WANProxyConfig()
{
	ASSERT(config_file_ == NULL);
	ASSERT(close_action_ == NULL);
	ASSERT(read_action_ == NULL);
	ASSERT(read_buffer_.empty());

	std::set<ProxyListener *>::iterator it;
	while ((it = listeners_.begin()) != listeners_.end()) {
		ProxyListener *listener = *it;
		delete listener;
		listeners_.erase(it);
	}
}

void
WANProxyConfig::close_complete(Event e, void *)
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
WANProxyConfig::read_complete(Event e, void *)
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
	DEBUG(log_) << "Reading configuration...";
	ASSERT(read_action_ == NULL);
	ASSERT(config_file_ != NULL);
	EventCallback *cb = callback(this, &WANProxyConfig::read_complete);
	read_action_ = config_file_->read(cb);
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

		std::vector<std::string> tokens;
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
	}
}

void
WANProxyConfig::parse(std::vector<std::string> tokens)
{
	if (tokens[0] == "proxy") {
		parse_proxy(tokens);
	} else if (tokens[0] == "proxy-socks") {
		parse_proxy_socks(tokens);
	} else {
		ERROR(log_) << "Unrecognized configuration directive: " << tokens[0];
	}
}

void
WANProxyConfig::parse_proxy(std::vector<std::string> tokens)
{
	if (tokens.size() != 10) {
		ERROR(log_) << "Wrong number of words in proxy (" << tokens.size() << ")";
		return;
	}

	std::string local_host = tokens[1];
	unsigned local_port = atoi(tokens[2].c_str());
	if (tokens[3] != "decoder") {
		ERROR(log_) << "Missing 'decoder' statement.";
		return;
	}

	XCodec *local_codec;
	if (tokens[4] == "xcodec")
		local_codec = codec_;
	else if (tokens[4] == "none")
		local_codec = NULL;
	else {
		ERROR(log_) << "Malformed decoder name.";
		return;
	}

	if (tokens[5] != "to") {
		ERROR(log_) << "Missing 'to' statement.";
		return;
	}

	std::string remote_host = tokens[6];
	unsigned remote_port = atoi(tokens[7].c_str());
	if (tokens[8] != "encoder") {
		ERROR(log_) << "Missing 'encoder' statement.";
		return;
	}

	XCodec *remote_codec;
	if (tokens[9] == "xcodec")
		remote_codec = codec_;
	else if (tokens[9] == "none")
		remote_codec = NULL;
	else {
		ERROR(log_) << "Malformed encoder name.";
		return;
	}

	INFO(log_) << "Starting proxy from " << local_host << ':' << local_port << " to " << remote_host << ':' << remote_port;

	ProxyListener *listener = new ProxyListener(local_codec,
						    remote_codec,
						    local_host,
						    local_port,
						    remote_host,
						    remote_port);
	listeners_.insert(listener);
}

void
WANProxyConfig::parse_proxy_socks(std::vector<std::string> tokens)
{
	if (tokens.size() != 3) {
		ERROR(log_) << "Wrong number of words in proxy-socks (" << tokens.size() << ")";
		return;
	}

	std::string local_host = tokens[1];
	unsigned local_port = atoi(tokens[2].c_str());

	INFO(log_) << "Starting socks-proxy on " << local_host << ':' << local_port;

	ProxySocksListener *listener = new ProxySocksListener(local_host, local_port);
	socks_listeners_.insert(listener);
}

bool
WANProxyConfig::configure(XCodec *codec, const std::string& name)
{
	INFO(log_) << "Configuring proxies.";

	if (config_file_ != NULL || codec_ != NULL) {
		ERROR(log_) << "Proxies already configured.";
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
