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

#ifndef	CONFIG_CONFIG_H
#define	CONFIG_CONFIG_H

#include <map>

class ConfigClass;
class ConfigClassInstance;
class ConfigExporter;
struct ConfigObject;

class Config {
	typedef	std::map<std::pair<ConfigObject *, std::string>, std::string> object_field_string_map_t;

	LogHandle log_;
	std::map<std::string, ConfigClass *> class_map_;
	std::map<std::string, ConfigObject *> object_map_;
	object_field_string_map_t field_strings_map_;

public:
	Config(void)
	: log_("/config"),
	  class_map_(),
	  object_map_()
	{ }

	~Config()
	{
#if 0 /* XXX NOTYET */
		ASSERT(log_, class_map_.empty());
		ASSERT(log_, object_map_.empty());
#endif
	}

	bool activate(const std::string&);
	bool create(const std::string&, const std::string&);
	bool set(const std::string&, const std::string&, const std::string&);

	void import(ConfigClass *);

	void marshall(ConfigExporter *) const;

	ConfigObject *lookup(const std::string& oname) const
	{
		std::map<std::string, ConfigObject *>::const_iterator omit;

		omit = object_map_.find(oname);
		if (omit == object_map_.end())
			return (NULL);
		return (omit->second);
	}
};

#endif /* !CONFIG_CONFIG_H */
