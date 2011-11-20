#include <common/buffer.h>

#include <config/config.h>
#include <config/config_class.h>
#include <config/config_exporter.h>
#include <config/config_object.h>
#include <config/config_type.h>
#include <config/config_type_pointer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>
#include <io/pipe/splice.h>

#include "monitor_client.h"

class HTMLConfigExporter : public ConfigExporter {
	std::string select_;
public:
	std::ostringstream os_;

	HTMLConfigExporter(const std::string& select)
	: select_(select),
	  os_()
	{ }

	~HTMLConfigExporter()
	{ }

	void config(const Config *c)
	{
		if (select_ == "")
			os_ << "<h1>Configuration</h1>";
		else
			os_ << "<h1>Configuration - " + select_ + "</h1>";
		c->marshall(this);
		os_ << "<hr />";
	}

	void field(const ConfigValue *cv, const std::string& name)
	{
		os_ << "<table>";
		os_ << "<tr><th>field</th><th>type</th><th>value</th></tr>";
		os_ << "<tr><td>" << name << "</td><td>" << cv->type_->name() << "</td><td>";
		cv->marshall(this);
		os_ << "</td></tr>";
		os_ << "</table>";
	}

	void object(const ConfigClass *cc, const ConfigObject *co)
	{
		if (select_ != "" && select_ != co->name())
			return;
		os_ << "<table>";
		os_ << "<tr><th>object</th><th>class</th><th>value</th></tr>";
		os_ << "<tr><td><a href=\"/" << co->name() << "\">" << co->name() << "</a></td><td>" << cc->name() << "</td><td>";
		cc->marshall(this, co);
		os_ << "</td></tr>";
		os_ << "</table>";
	}

	void value(const ConfigValue *cv, const std::string& val)
	{
		ConfigTypePointer *ct = dynamic_cast<ConfigTypePointer *>(cv->type_);
		if (ct != NULL) {
			ConfigObject *co;
			if (ct->get(cv, &co)) {
				if (co == NULL)
					os_ << "<tt>None</tt>";
				else
					os_ << "<a href=\"/" << co->name() << "\">" << co->name() << "</a>";
				return;
			}
		}
		os_ << "<tt>" << val << "</tt>";
	}
};


void
MonitorClient::handle_request(const std::string& method, const std::string& uri, HTTPProtocol::Message)
{
	if (method != "GET") {
		pipe_->send_response(HTTPProtocol::BadRequest, "Unsupported method.");
		return;
	}

	if (uri[0] != '/') {
		pipe_->send_response(HTTPProtocol::NotFound, "Invalid URI.");
		return;
	}

	std::vector<Buffer> path_components = Buffer(uri).split('/', false);
	std::string select;
	if (!path_components.empty()) {
		if (path_components.size() != 1) {
			pipe_->send_response(HTTPProtocol::NotFound, "Too many path components in URI.");
			return;
		}
		path_components[0].extract(select);
	}

	HTMLConfigExporter exporter(select);
	exporter.config(config_);
	pipe_->send_response(HTTPProtocol::OK, "<html><head><title>WANProxy Configuration Monitor</title><style type=\"text/css\">body { font-family: sans-serif; } td, th { vertical-align: text-top; text-align: left; }</style></head><body>" + exporter.os_.str() + "</body></html>", "text/html");
}
