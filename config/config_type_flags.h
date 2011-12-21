#ifndef	CONFIG_CONFIG_TYPE_FLAGS_H
#define	CONFIG_CONFIG_TYPE_FLAGS_H

#include <map>

#include <config/config_exporter.h>
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
	std::map<const ConfigValue *, T> flags_;
	std::map<std::string, T> flag_map_;
public:
	ConfigTypeFlags(const std::string& xname, struct Mapping *mappings)
	: ConfigType(xname),
	  flags_(),
	  flag_map_()
	{
		ASSERT("/config/type/flags", mappings != NULL);
		while (mappings->string_ != NULL) {
			flag_map_[mappings->string_] = mappings->flag_;
			mappings++;
		}
	}

	~ConfigTypeFlags()
	{
		flags_.clear();
	}

	bool get(const ConfigValue *cv, T *flagsp) const
	{
		typename std::map<const ConfigValue *, T>::const_iterator it;
		it = flags_.find(cv);
		if (it == flags_.end()) {
			ERROR("/config/type/flags") << "Value not set.";
			return (false);
		}
		*flagsp = it->second;
		return (true);
	}

	void marshall(ConfigExporter *exp, const ConfigValue *cv) const
	{
		T flags;
		if (!get(cv, &flags))
			HALT("/config/type/flags") << "Trying to marshall unset value.";

		std::string str;
		typename std::map<std::string, T>::const_iterator it;
		for (it = flag_map_.begin(); it != flag_map_.end(); ++it) {
			if ((it->second & flags) == 0)
				continue;
			flags &= ~it->second;
			if (!str.empty())
				str += "|";
			str += it->first;
		}
		if (flags != 0)
			HALT("/config/type/flags") << "Trying to marshall unknown flags.";
		if (flags == 0 && str.empty())
			str = "0";
		exp->value(cv, str);
	}

	bool set(const ConfigValue *cv, const std::string& vstr)
	{
		if (flag_map_.find(vstr) == flag_map_.end()) {
			ERROR("/config/type/flags") << "Invalid value (" << vstr << ")";
			return (false);
		}

		flags_[cv] |= flag_map_[vstr];

		return (true);
	}
};

#endif /* !CONFIG_CONFIG_TYPE_FLAGS_H */
