/*
 * Copyright (c) 2015 Juli Mallett. All rights reserved.
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

#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_CACHE_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_CACHE_H

#include <config/config_type_size.h>
#include <config/config_type_pointer.h>
#include <config/config_type_string.h>

#include "wanproxy_config_type_cache.h"

class XCodecCache;

class WANProxyConfigClassCache : public ConfigClass {
public:
	struct Instance : public ConfigClassInstance {
		XCodecCache *cache_;
		WANProxyConfigCache type_;
		std::string uuid_;
		intmax_t size_;
		std::string path_;
		ConfigObject *primary_;
		ConfigObject *secondary_;

		Instance(void)
		: cache_(),
		  type_(WANProxyConfigCacheMemory),
		  uuid_(""),
		  size_(0),
		  path_(""),
		  primary_(NULL),
		  secondary_(NULL)
		{ }

		bool activate(const ConfigObject *);
	};

	WANProxyConfigClassCache(void)
	: ConfigClass("cache", new ConstructorFactory<ConfigClassInstance, Instance>)
	{
		add_member("type", &wanproxy_config_type_cache, &Instance::type_);
		add_member("uuid", &config_type_string, &Instance::uuid_);
		add_member("size", &config_type_size, &Instance::size_);
		add_member("path", &config_type_string, &Instance::path_);
		add_member("primary", &config_type_pointer, &Instance::primary_);
		add_member("secondary", &config_type_pointer, &Instance::secondary_);
	}

	~WANProxyConfigClassCache()
	{ }
};

extern WANProxyConfigClassCache wanproxy_config_class_cache;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_CACHE_H */
