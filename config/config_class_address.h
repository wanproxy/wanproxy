/*
 * Copyright (c) 2009-2013 Juli Mallett. All rights reserved.
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

#ifndef	CONFIG_CONFIG_CLASS_ADDRESS_H
#define	CONFIG_CONFIG_CLASS_ADDRESS_H

#include <config/config_type_address_family.h>
#include <config/config_type_string.h>

class ConfigClassAddress : public ConfigClass {
public:
	struct Instance : public ConfigClassInstance {
		SocketAddressFamily family_;
		std::string host_;
		std::string port_;
		std::string path_;

		Instance(void)
		: family_(SocketAddressFamilyUnspecified),
		  host_(""),
		  port_(""),
		  path_("")
		{
		}

		bool activate(const ConfigObject *);
	};

	ConfigClassAddress(const std::string& xname = "address")
	: ConfigClass(xname, new ConstructorFactory<ConfigClassInstance, Instance>)
	{
		add_member("family", &config_type_address_family, &Instance::family_);
		add_member("host", &config_type_string, &Instance::host_);
		add_member("port", &config_type_string, &Instance::port_); /* XXX enum?  */
		add_member("path", &config_type_string, &Instance::path_);
	}

	~ConfigClassAddress()
	{ }
};

extern ConfigClassAddress config_class_address;

#endif /* !CONFIG_CONFIG_CLASS_ADDRESS_H */
