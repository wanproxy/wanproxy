#ifndef	CONFIG_CONFIG_TYPE_ENUM_H
#define	CONFIG_CONFIG_TYPE_ENUM_H

#include <map>

#include <config/config_exporter.h>
#include <config/config_type.h>

template<typename E>
class ConfigTypeEnum : public ConfigType {
public:
	struct Mapping {
		const char *string_;
		E enum_;
	};
private:
	std::map<std::string, E> enum_map_;
public:
	ConfigTypeEnum(const std::string& xname, struct Mapping *mappings)
	: ConfigType(xname),
	  enum_map_()
	{
		ASSERT("/config/type/enum", mappings != NULL);
		while (mappings->string_ != NULL) {
			enum_map_[mappings->string_] = mappings->enum_;
			mappings++;
		}
	}

	~ConfigTypeEnum()
	{ }

	void marshall(ConfigExporter *exp, const E *enump) const
	{
		E xenum = *enump;

		typename std::map<std::string, E>::const_iterator it;
		for (it = enum_map_.begin(); it != enum_map_.end(); ++it) {
			if (it->second != xenum)
				continue;
			exp->value(this, it->first);
			return;
		}
		HALT("/config/type/enum") << "Trying to marshall unknown enum.";
	}

	bool set(ConfigObject *, const std::string& vstr, E *enump)
	{
		if (enum_map_.find(vstr) == enum_map_.end()) {
			ERROR("/config/type/enum") << "Invalid value (" << vstr << ")";
			return (false);
		}

		*enump = enum_map_[vstr];

		return (true);
	}
};

#endif /* !CONFIG_CONFIG_TYPE_ENUM_H */
