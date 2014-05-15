/*
 * Copyright (c) 2009-2014 Juli Mallett. All rights reserved.
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

#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PROXY_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PROXY_H

#include <config/config_type_pointer.h>
#include <config/config_type_string.h>

#include "wanproxy_config_type_proxy_type.h"

class ProxyListener;

class WANProxyConfigClassProxy : public ConfigClass {
	struct Instance : public ConfigClassInstance {
		WANProxyConfigProxyType type_;
		ConfigObject *interface_;
		ConfigObject *interface_codec_;
		ConfigObject *peer_;
		ConfigObject *peer_codec_;
		std::string server_host_key_;

		Instance(void)
		: type_(WANProxyConfigProxyTypeTCPTCP),
		  interface_(NULL),
		  interface_codec_(NULL),
		  peer_(NULL),
		  peer_codec_(NULL),
		  server_host_key_("")
		{ }

		bool activate(const ConfigObject *);
	};
public:
	WANProxyConfigClassProxy(void)
	: ConfigClass("proxy", new ConstructorFactory<ConfigClassInstance, Instance>)
	{
		add_member("type", &wanproxy_config_type_proxy_type, &Instance::type_);
		add_member("interface", &config_type_pointer, &Instance::interface_);
		add_member("interface_codec", &config_type_pointer, &Instance::interface_codec_);
		add_member("peer", &config_type_pointer, &Instance::peer_);
		add_member("peer_codec", &config_type_pointer, &Instance::peer_codec_);
		add_member("server_host_key", &config_type_string, &Instance::server_host_key_);
	}

	/* XXX So wrong.  */
	~WANProxyConfigClassProxy()
	{ }
};

extern WANProxyConfigClassProxy wanproxy_config_class_proxy;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PROXY_H */
