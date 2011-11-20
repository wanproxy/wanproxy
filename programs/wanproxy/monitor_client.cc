#include <common/buffer.h>

#include <config/config.h>
#include <config/config_class.h>
#include <config/config_exporter.h>
#include <config/config_object.h>
#include <config/config_type.h>

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
public:
	std::ostringstream os_;

	HTMLConfigExporter(void)
	: os_()
	{ }

	~HTMLConfigExporter()
	{ }

	void config(const Config *c)
	{
		os_ << "<h1>Configuration</h1>";
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
		os_ << "<table>";
		os_ << "<tr><th>object</th><th>name</th><th>class</th><th>value</th></tr>";
		os_ << "<tr><td>" << co->name() << "</td><td>" << (void *)co << "</td><td>" << cc->name() << "</td><td>";
		cc->marshall(this, co);
		os_ << "</td></tr>";
		os_ << "</table>";
	}

	void value(const ConfigValue *, const std::string& val)
	{
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

	if (uri != "/") {
		pipe_->send_response(HTTPProtocol::NotFound, "No such URI.");
		return;
	}

	HTMLConfigExporter exporter;
	exporter.config(config_);
	pipe_->send_response(HTTPProtocol::OK, "<html><head><title>WANProxy Configuration Monitor</title><style type=\"text/css\">body { font-family: sans-serif; } td, th { vertical-align: text-top; text-align: left; }</style></head><body>" + exporter.os_.str() + "</body></html>", "text/html");
}
