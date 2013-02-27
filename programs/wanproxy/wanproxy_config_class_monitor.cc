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

#include <config/config_class.h>
#include <config/config_object.h>

#include <event/action.h>
#include <event/event.h>
#include <event/event_callback.h>
#include <event/event_system.h>

#include <http/http_server.h>

#include <io/socket/socket_types.h>

#include "monitor_client.h"
#include "wanproxy_config_class_interface.h"
#include "wanproxy_config_class_monitor.h"

WANProxyConfigClassMonitor wanproxy_config_class_monitor;

bool
WANProxyConfigClassMonitor::Instance::activate(const ConfigObject *co)
{
	WANProxyConfigClassInterface::Instance *interface =
		dynamic_cast<WANProxyConfigClassInterface::Instance *>(interface_->instance_);
	if (interface == NULL)
		return (false);

	if (interface->host_ == "" || interface->port_ == "")
		return (false);
	std::string interface_address = '[' + interface->host_ + ']' + ':' + interface->port_;
	new HTTPServer<TCPServer, MonitorClient, Config *>(co->config_, interface->family_, interface_address);

	return (true);
}
