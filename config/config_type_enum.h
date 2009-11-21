#ifndef	CONFIG_TYPE_ENUM_H
#define	CONFIG_TYPE_ENUM_H

#include <map>

#include <config/config_type.h>

struct ConfigValue;

template<typename E>
class ConfigTypeEnum : public ConfigType {
public:
	struct Mapping {
		const char *string_;
		E enum_;
	};
private:
	std::map<ConfigValue *, E> enums_;
	std::map<std::string, E> enum_map_;
public:
	ConfigTypeEnum(const std::string& name, struct Mapping *mappings)
	: ConfigType(name),
	  enums_()
	{
		ASSERT(mappings != NULL);
		while (mappings->string_ != NULL) {
			enum_map_[mappings->string_] = mappings->enum_;
			mappings++;
		}
	}

	~ConfigTypeEnum()
	{
		enums_.clear();
	}

	bool get(ConfigValue *cv, E *enump)
	{
		if (enums_.find(cv) == enums_.end()) {
			ERROR("/config/type/enum") << "Value not set.";
			return (false);
		}
		*enump = enums_[cv];
		return (true);
	}

	bool set(ConfigValue *cv, const std::string& vstr)
	{
		if (enums_.find(cv) != enums_.end()) {
			ERROR("/config/type/enum") << "Value already set.";
			return (false);
		}

		if (enum_map_.find(vstr) == enum_map_.end()) {
			ERROR("/config/type/enum") << "Invalid value (" << vstr << ")";
			return (false);
		}

		enums_[cv] = enum_map_[vstr];

		return (true);
	}
};

#endif /* !CONFIG_TYPE_ENUM_H */
