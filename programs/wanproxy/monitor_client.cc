/*
 * Copyright (c) 2011-2013 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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

	void field(const ConfigClassInstance *inst, const ConfigClassMember *m, const std::string& name)
	{
		ConfigType *ct = m->type();
		os_ << "<tr><td colspan=\"2\" /><td>" << name << "</td><td>" << ct->name() << "</td><td>";
		m->marshall(this, inst);
		os_ << "</td></tr>";
	}

	void object(const ConfigObject *co, const std::string& name)
	{
		if (select_ != "" && select_ != name)
			return;
		os_ << "<tr><th>object</th><th>class</th><th>field</th><th>type</th><th>value</th></tr>";
		os_ << "<tr><td><a href=\"/object/" << name << "\">" << name << "</a></td><td>" << co->class_->name() << "</td><td colspan=\"3\" /></tr>";
		co->marshall(this);
	}

	void value(const ConfigType *ct, const std::string& val)
	{
		const ConfigTypePointer *ctp = dynamic_cast<const ConfigTypePointer *>(ct);
		if (ctp != NULL) {
			if (val != "None") {
				os_ << "<a href=\"/object/" << val << "\">" << val << "</a>";
				return;
			}
		}
		os_ << "<tt>" << val << "</tt>";
	}
};

class TextConfigExporter : public ConfigExporter {
	std::string select_;
public:
	std::ostringstream os_;

	TextConfigExporter(const std::string& select)
	: select_(select),
	  os_()
	{ }

	~TextConfigExporter()
	{ }

	void config(const Config *c)
	{
		if (select_ == "")
			os_ << "config:\r\n";
		else
			os_ << "config " + select_ + ":\r\n";
		c->marshall(this);
	}

	void field(const ConfigClassInstance *inst, const ConfigClassMember *m, const std::string& name)
	{
		ConfigType *ct = m->type();
		os_ << "  field: " << name << "; type: " << ct->name() << ";\r\n";
		m->marshall(this, inst);
	}

	void object(const ConfigObject *co, const std::string& name)
	{
		if (select_ != "" && select_ != name)
			return;
		os_ << " object: " << name << "; class: " << co->class_->name() << ";\r\n";
		co->marshall(this);
	}

	void value(const ConfigType *, const std::string& val)
	{
		os_ << "   value: " << val << ";\r\n";
	}
};

void
MonitorClient::handle_request(const std::string& method, const std::string& uri, HTTPProtocol::Request)
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

	bool text = false;
	if (!path_components.empty() && path_components[0].equal(":text")) {
		text = true;
		path_components.erase(path_components.begin());
	}

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

	if (text) {
		TextConfigExporter exporter(select);
		exporter.config(config_);
		pipe_->send_response(HTTPProtocol::OK, exporter.os_.str());
		return;
	}
	HTMLConfigExporter exporter(select);
	exporter.config(config_);
	pipe_->send_response(HTTPProtocol::OK, "<html><head><title>WANProxy Monitor</title><style type=\"text/css\">body { font-family: sans-serif; } td, th { vertical-align: text-top; text-align: left; }</style></head><body>" + exporter.os_.str() + "</body></html>", "text/html");
}
