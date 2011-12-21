#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_H

class Config;

class WANProxyConfig {
	LogHandle log_;
	Config *config_;
public:
	WANProxyConfig(void);
	~WANProxyConfig();

private:
	bool parse(std::deque<std::string>&);

	void parse_activate(std::deque<std::string>&);
	void parse_create(std::deque<std::string>&);
	void parse_set(std::deque<std::string>&);

public:
	bool configure(const std::string&);
};

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_H */
