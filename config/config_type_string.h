#ifndef	CONFIG_TYPE_STRING_H
#define	CONFIG_TYPE_STRING_H

#include <map>

#include <config/config_exporter.h>
#include <config/config_type.h>

struct ConfigValue;

class ConfigTypeString : public ConfigType {
	std::map<const ConfigValue *, std::string> strings_;
public:
	ConfigTypeString(void)
	: ConfigType("string"),
	  strings_()
	{ }

	~ConfigTypeString()
	{
		strings_.clear();
	}

	bool get(const ConfigValue *cv, std::string *strp) const
	{
		std::map<const ConfigValue *, std::string>::const_iterator it;
		it = strings_.find(cv);
		if (it == strings_.end()) {
			ERROR("/config/type/string") << "Value not set.";
			return (false);
		}
		*strp = it->second;
		return (true);
	}

	void marshall(ConfigExporter *exp, const ConfigValue *cv) const
	{
		std::string str;
		if (!get(cv, &str))
			HALT("/config/type/string") << "Trying to marshall unset value.";

		exp->value(cv, str);
	}

	bool set(const ConfigValue *cv, const std::string& vstr)
	{
		if (strings_.find(cv) != strings_.end()) {
			ERROR("/config/type/string") << "Value already set.";
			return (false);
		}

		if (*vstr.begin() != '"') {
			ERROR("/config/type/string") << "String does not begin with '\"'.";
			return (false);
		}
		if (*vstr.rbegin() != '"') {
			ERROR("/config/type/string") << "String does not end with '\"'.";
			return (false);
		}

		std::string str(vstr.begin() + 1, vstr.end() - 1);
		if (std::find(str.begin(), str.end(), '"') != str.end()) {
			ERROR("/config/type/string") << "String has '\"' other than at beginning or end.";
			return (false);
		}

		strings_[cv] = str;
		return (true);
	}
};

extern ConfigTypeString config_type_string;

#endif /* !CONFIG_TYPE_STRING_H */
