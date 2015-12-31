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

#ifndef	CONFIG_CONFIG_TYPE_ENUM_H
#define	CONFIG_CONFIG_TYPE_ENUM_H

#include <map>

#include <config/config_exporter.h>
#include <config/config_type.h>

template<typename E>
class ConfigTypeEnum : public ConfigType {
public:
	struct Mapping {
		const char *string_;
		E enum_;
	};
private:
	std::map<std::string, E> enum_map_;
public:
	ConfigTypeEnum(const std::string& xname, struct Mapping *mappings)
	: ConfigType(xname),
	  enum_map_()
	{
		ASSERT_NON_NULL("/config/type/enum", mappings);
		while (mappings->string_ != NULL) {
			enum_map_[mappings->string_] = mappings->enum_;
			mappings++;
		}
	}

	~ConfigTypeEnum()
	{ }

	void marshall(ConfigExporter *exp, const E *enump) const
	{
		E xenum = *enump;

		typename std::map<std::string, E>::const_iterator it;
		for (it = enum_map_.begin(); it != enum_map_.end(); ++it) {
			if (it->second != xenum)
				continue;
			exp->value(this, it->first);
			return;
		}
		HALT("/config/type/enum") << "Trying to marshall unknown enum.";
	}

	bool set(ConfigObject *, const std::string& vstr, E *enump)
	{
		if (enum_map_.find(vstr) == enum_map_.end()) {
			ERROR("/config/type/enum") << "Invalid value (" << vstr << ")";
			return (false);
		}

		*enump = enum_map_[vstr];

		return (true);
	}
};

#endif /* !CONFIG_CONFIG_TYPE_ENUM_H */
