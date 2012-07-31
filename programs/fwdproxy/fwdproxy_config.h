#ifndef	PROGRAMS_FWDPROXY_FWDPROXY_CONFIG_H
#define	PROGRAMS_FWDPROXY_FWDPROXY_CONFIG_H

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

#endif /* !PROGRAMS_FWDPROXY_FWDPROXY_CONFIG_H */
