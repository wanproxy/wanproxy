#ifndef	CONFIG_TYPE_FLAGS_H
#define	CONFIG_TYPE_FLAGS_H

#include <map>

#include <config/config_type.h>

struct ConfigValue;

template<typename T>
class ConfigTypeFlags : public ConfigType {
public:
	struct Mapping {
		const char *string_;
		T flag_;
	};
private:
	std::map<ConfigValue *, T> flags_;
	std::map<std::string, T> flag_map_;
public:
	ConfigTypeFlags(const std::string& name, struct Mapping *mappings)
	: ConfigType(name),
	  flags_(),
	  flag_map_()
	{
		ASSERT(mappings != NULL);
		while (mappings->string_ != NULL) {
			flag_map_[mappings->string_] = mappings->flag_;
			mappings++;
		}
	}

	~ConfigTypeFlags()
	{
		flags_.clear();
	}

	bool get(ConfigValue *cv, T *flagsp)
	{
		if (flags_.find(cv) == flags_.end()) {
			ERROR("/config/type/flags") << "Value not set.";
			return (false);
		}
		*flagsp = flags_[cv];
		return (true);
	}

	bool set(ConfigValue *cv, const std::string& vstr)
	{
		if (flag_map_.find(vstr) == flag_map_.end()) {
			ERROR("/config/type/flags") << "Invalid value (" << vstr << ")";
			return (false);
		}

		flags_[cv] |= flag_map_[vstr];

		return (true);
	}
};

#endif /* !CONFIG_TYPE_FLAGS_H */
