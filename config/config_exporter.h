#ifndef	CONFIG_CONFIG_EXPORTER_H
#define	CONFIG_CONFIG_EXPORTER_H

class ConfigClass;
class ConfigObject;
struct ConfigValue;

class ConfigExporter {
protected:
	ConfigExporter(void)
	{ }

	virtual ~ConfigExporter()
	{ }

public:
	virtual void field(const ConfigValue *, const std::string&) = 0;
	virtual void object(const ConfigClass *, const ConfigObject *) = 0;
	virtual void value(const ConfigValue *, const std::string&) = 0;
};

#endif /* !CONFIG_CONFIG_EXPORTER_H */
