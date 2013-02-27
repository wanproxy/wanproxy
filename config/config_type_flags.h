#ifndef	CONFIG_CONFIG_TYPE_FLAGS_H
#define	CONFIG_CONFIG_TYPE_FLAGS_H

#include <map>

#include <config/config_exporter.h>
#include <config/config_type.h>

template<typename T>
class ConfigTypeFlags : public ConfigType {
public:
	struct Mapping {
		const char *string_;
		T flag_;
	};
private:
	std::map<std::string, T> flag_map_;
public:
	ConfigTypeFlags(const std::string& xname, struct Mapping *mappings)
	: ConfigType(xname),
	  flag_map_()
	{
		ASSERT("/config/type/flags", mappings != NULL);
		while (mappings->string_ != NULL) {
			flag_map_[mappings->string_] = mappings->flag_;
			mappings++;
		}
	}

	~ConfigTypeFlags()
	{ }

	void marshall(ConfigExporter *exp, const T *flagsp) const
	{
		T flags = *flagsp;

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
		exp->value(this, str);
	}

	bool set(ConfigObject *, const std::string& vstr, T *flagsp)
	{
		if (flag_map_.find(vstr) == flag_map_.end()) {
			ERROR("/config/type/flags") << "Invalid value (" << vstr << ")";
			return (false);
		}

		*flagsp |= flag_map_[vstr];

		return (true);
	}
};

#endif /* !CONFIG_CONFIG_TYPE_FLAGS_H */
