#ifndef	CONFIG_CONFIG_CLASS_H
#define	CONFIG_CONFIG_CLASS_H

#include <map>

#include <common/factory.h>

#include <config/config_object.h>

class Config;
class ConfigExporter;
class ConfigType;

/*
 * A base type for class instances.
 */
class ConfigClassInstance {
protected:
	ConfigClassInstance(void)
	{ }
public:
	virtual ~ConfigClassInstance()
	{ }

	virtual bool activate(const ConfigObject *) = 0;
};

class ConfigClassMember {
protected:
	ConfigClassMember(void)
	{ }
public:
	virtual ~ConfigClassMember()
	{ }

	virtual void marshall(ConfigExporter *, const ConfigClassInstance *) const = 0;
	virtual bool set(ConfigObject *, const std::string&) const = 0;
	virtual ConfigType *type(void) const = 0;
};

class ConfigClass {
	friend class Config;
	friend struct ConfigObject;

	std::string name_;
	Factory<ConfigClassInstance> *factory_;
	std::map<std::string, const ConfigClassMember *> members_;
protected:
	ConfigClass(const std::string& xname, Factory<ConfigClassInstance> *factory)
	: name_(xname),
	  factory_(factory),
	  members_()
	{ }

	virtual ~ConfigClass();

protected:
	template<typename Tc, typename Tf, typename Ti>
	void add_member(const std::string& mname, Tc type, Tf Ti::*fieldp)
	{
		struct TypedConfigClassMember : public ConfigClassMember {
			Tc config_type_;
			Tf Ti::*config_field_;

			TypedConfigClassMember(Tc config_type, Tf Ti::*config_field)
			: config_type_(config_type),
			  config_field_(config_field)
			{ }

			void marshall(ConfigExporter *exp, const ConfigClassInstance *instance) const
			{
				const Ti *inst = dynamic_cast<const Ti *>(instance);
				ASSERT("/config/class/field", inst != NULL);
				config_type_->marshall(exp, &(inst->*config_field_));
			}

			bool set(ConfigObject *co, const std::string& vstr) const
			{
				Ti *inst = dynamic_cast<Ti *>(co->instance_);
				ASSERT("/config/class/field", inst != NULL);
				return (config_type_->set(co, vstr, &(inst->*config_field_)));
			}

			ConfigType *type(void) const
			{
				return (config_type_);
			}
		};

		ASSERT("/config/class/" + name_, members_.find(mname) == members_.end());
		members_[mname] = new TypedConfigClassMember(type, fieldp);
	}

private:
	ConfigClassInstance *allocate(void) const
	{
		return (factory_->create());
	}

	bool set(ConfigObject *, const std::string&, const std::string&) const;

public:
	void marshall(ConfigExporter *, const ConfigClassInstance *) const;

	std::string name(void) const
	{
		return (name_);
	}
};

#endif /* !CONFIG_CONFIG_CLASS_H */
