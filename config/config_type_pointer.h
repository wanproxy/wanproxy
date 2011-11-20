#ifndef	CONFIG_TYPE_POINTER_H
#define	CONFIG_TYPE_POINTER_H

#include <map>

#include <config/config_object.h>
#include <config/config_type.h>

struct ConfigValue;

class ConfigTypePointer : public ConfigType {
	std::map<const ConfigValue *, ConfigObject *> pointers_;
public:
	ConfigTypePointer(void)
	: ConfigType("pointer"),
	  pointers_()
	{ }

	~ConfigTypePointer()
	{
		pointers_.clear();
	}

	bool get(const ConfigValue *cv, ConfigObject **cop) const
	{
		std::map<const ConfigValue *, ConfigObject *>::const_iterator it;

		it = pointers_.find(cv);
		if (it == pointers_.end()) {
			ERROR("/config/type/pointer") << "Value not set.";
			return (false);
		}

		ConfigObject *co = it->second;
		if (co == NULL) {
			*cop = NULL;
			return (true);
		}

		*cop = co;

		return (true);
	}

	template<typename T>
	bool get(const ConfigValue *cv, ConfigObject **cop, T **ccp) const
	{
		std::map<const ConfigValue *, ConfigObject *>::const_iterator it;

		it = pointers_.find(cv);
		if (it == pointers_.end()) {
			ERROR("/config/type/pointer") << "Value not set.";
			return (false);
		}

		ConfigObject *co = it->second;
		if (co == NULL) {
			*cop = NULL;
			*ccp = NULL;
			return (true);
		}

#if !defined(__OpenBSD__)
		T *cc = co->coerce<T>();
#else
		T *cc;
		co->coerce(&cc);
#endif
		if (cc == NULL) {
			ERROR("/config/type/pointer") << "Pointer to object of wrong type.";
			return (false);
		}

		*cop = co;
		*ccp = cc;

		return (true);
	}

	void marshall(ConfigExporter *, const ConfigValue *) const;

	bool set(const ConfigValue *, const std::string&);
};

extern ConfigTypePointer config_type_pointer;

#endif /* !CONFIG_TYPE_POINTER_H */
