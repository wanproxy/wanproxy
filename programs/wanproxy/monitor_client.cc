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
			os_ << "<h1>Configuration - " << select_ << " [<a href=\"/\">up</a>]</h1>";
		os_ << "<table>";
		c->marshall(this);
		os_ << "</table>";
	}

	void field(const ConfigValue *cv, const std::string& name)
	{
		os_ << "<tr><td colspan=\"2\" /><td>" << name << "</td><td>" << cv->type_->name() << "</td><td>";
		cv->marshall(this);
		os_ << "</td></tr>";
	}

	void object(const ConfigClass *cc, const ConfigObject *co)
	{
		if (select_ != "" && select_ != co->name())
			return;
		os_ << "<tr><th>object</th><th>class</th><th>field</th><th>type</th><th>value</th></tr>";
		os_ << "<tr><td><a href=\"/object/" << co->name() << "\">" << co->name() << "</a></td><td>" << cc->name() << "</td><td colspan=\"3\" /></tr>";
		cc->marshall(this, co);
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
					os_ << "<a href=\"/object/" << co->name() << "\">" << co->name() << "</a>";
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
		switch (path_components.size()) {
		case 2:
			if (!path_components[0].equal("object")) {
				pipe_->send_response(HTTPProtocol::NotFound, "Unsupported URI class.");
				return;
			}
			path_components[1].extract(select);
			break;
		default:
			pipe_->send_response(HTTPProtocol::NotFound, "Wrong number of path components in URI.");
			return;
		}
	}

	HTMLConfigExporter exporter(select);
	exporter.config(config_);
	pipe_->send_response(HTTPProtocol::OK, "<html><head><title>WANProxy Configuration Monitor</title><style type=\"text/css\">body { font-family: sans-serif; } td, th { vertical-align: text-top; text-align: left; }</style></head><body>" + exporter.os_.str() + "</body></html>", "text/html");
}
