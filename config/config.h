#ifndef	CONFIG_H
#define	CONFIG_H

#include <map>

class ConfigClass;
class ConfigObject;

class Config {
	LogHandle log_;
	std::map<std::string, ConfigClass *> class_map_;
	std::map<std::string, ConfigObject *> object_map_;

public:
	Config(void);
	~Config();

	bool activate(const std::string&);
	bool create(const std::string&, const std::string&);
	bool set(const std::string&, const std::string&, const std::string&);

	void import(const std::string&, ConfigClass *);
};

#endif /* !CONFIG_H */
