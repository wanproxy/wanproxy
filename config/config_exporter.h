#ifndef	CONFIG_CONFIG_EXPORTER_H
#define	CONFIG_CONFIG_EXPORTER_H

class ConfigClass;
class ConfigClassInstance;
class ConfigClassMember;
struct ConfigObject;
class ConfigType;

class ConfigExporter {
protected:
	ConfigExporter(void)
	{ }

	virtual ~ConfigExporter()
	{ }

public:
	virtual void field(const ConfigClassInstance *, const ConfigClassMember *, const std::string&) = 0;
	virtual void object(const ConfigObject *, const std::string&) = 0;
	virtual void value(const ConfigType *, const std::string&) = 0;
};

#endif /* !CONFIG_CONFIG_EXPORTER_H */
