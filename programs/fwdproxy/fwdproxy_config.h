#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_H

class FWDProxyConfig {
	LogHandle log_;
public:
	FWDProxyConfig(void);
	~FWDProxyConfig();

private:
	bool parse(std::deque<std::string>&);

public:
	bool configure(const std::string&);
};

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_H */
