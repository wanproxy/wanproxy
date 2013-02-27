#ifndef	CONFIG_CONFIG_TYPE_STRING_H
#define	CONFIG_CONFIG_TYPE_STRING_H

#include <map>

#include <config/config_exporter.h>
#include <config/config_type.h>

class ConfigTypeString : public ConfigType {
public:
	ConfigTypeString(void)
	: ConfigType("string")
	{ }

	~ConfigTypeString()
	{ }

	void marshall(ConfigExporter *exp, const std::string *stringp) const
	{
		exp->value(this, *stringp);
	}

	bool set(ConfigObject *, const std::string& vstr, std::string *stringp)
	{
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

		*stringp = str;
		return (true);
	}
};

extern ConfigTypeString config_type_string;

#endif /* !CONFIG_CONFIG_TYPE_STRING_H */
