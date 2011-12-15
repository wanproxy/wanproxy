#ifndef	CONFIG_OBJECT_H
#define	CONFIG_OBJECT_H

#include <map>

#include <config/config_value.h>

class Config;
class ConfigClass;
class ConfigExporter;

class ConfigObject {
	friend class Config;
	friend class ConfigClass;

	std::string name_;
	Config *config_;
	ConfigClass *class_;
	std::map<std::string, ConfigValue *> members_;

public:
	ConfigObject(const std::string& xname, Config *config, ConfigClass *cc)
	: name_(xname),
	  config_(config),
	  class_(cc),
	  members_()
	{ }

	~ConfigObject();

	template<typename T>
	T *coerce(void)
	{
		T *cc = dynamic_cast<T *>(class_);
		if (cc == NULL)
			return (NULL);
		return (cc);
	}

	template<typename T>
	ConfigValue *get(const std::string& mname, T **ctp) const
	{
		std::map<std::string, ConfigValue *>::const_iterator it;

		/*
		 * Some versions of GCC are in fact so broken that we have to
		 * set *ctp to NULL in *all* of these cases.  Have tried shaving
		 * the yak and gotten only despair.
		 */
		it = members_.find(mname);
		if (it == members_.end()) {
			*ctp = NULL; /* XXX GCC -Wuninitialized.  */
			return (NULL);
		}

		ConfigValue *cv = it->second;
		if (cv == NULL) {
			*ctp = NULL; /* XXX GCC -Wuninitialized.  */
			return (NULL);
		}

		T *ct = dynamic_cast<T *>(cv->type_);
		if (ct == NULL) {
			*ctp = NULL; /* XXX GCC -Wuninitialized.  */
			return (NULL);
		}

		*ctp = ct;
		return (cv);
	}

	bool has(const std::string& mname) const
	{
		std::map<std::string, ConfigValue *>::const_iterator it;

		it = members_.find(mname);
		if (it == members_.end())
			return (false);
		return (true);
	}

	void marshall(ConfigExporter *) const;

	std::string name(void) const
	{
		return (name_);
	}
};

#endif /* !CONFIG_OBJECT_H */
