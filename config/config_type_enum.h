#ifndef	CONFIG_TYPE_ENUM_H
#define	CONFIG_TYPE_ENUM_H

#include <map>

#include <config/config_exporter.h>
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
	std::map<const ConfigValue *, E> enums_;
	std::map<std::string, E> enum_map_;
public:
	ConfigTypeEnum(const std::string& name, struct Mapping *mappings)
	: ConfigType(name),
	  enums_(),
	  enum_map_()
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

	bool get(const ConfigValue *cv, E *enump) const
	{
		typename std::map<const ConfigValue *, E>::const_iterator it;
		it = enums_.find(cv);
		if (it == enums_.end()) {
			ERROR("/config/type/enum") << "Value not set.";
			return (false);
		}
		*enump = it->second;
		return (true);
	}

	void marshall(ConfigExporter *exp, const ConfigValue *cv) const
	{
		E xenum;
		if (!get(cv, &xenum))
			HALT("/config/type/enum") << "Trying to marshall unset value.";

		typename std::map<std::string, E>::const_iterator it;
		for (it = enum_map_.begin(); it != enum_map_.end(); ++it) {
			if (it->second != xenum)
				continue;
			exp->value(cv, it->first);
			return;
		}
		HALT("/config/type/enum") << "Trying to marshall unknown enum.";
	}

	bool set(const ConfigValue *cv, const std::string& vstr)
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
