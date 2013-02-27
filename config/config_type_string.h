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

#ifndef	CONFIG_CONFIG_TYPE_STRING_H
#define	CONFIG_CONFIG_TYPE_STRING_H

#include <map>

#include <config/config_exporter.h>
#include <config/config_type.h>

class ConfigTypeString : public ConfigType {
public:
	ConfigTypeString(void)
	: ConfigType("string")
	{ }

	~ConfigTypeString()
	{ }

	void marshall(ConfigExporter *exp, const std::string *stringp) const
	{
		exp->value(this, *stringp);
	}

	bool set(ConfigObject *, const std::string& vstr, std::string *stringp)
	{
		if (*vstr.begin() != '"') {
			ERROR("/config/type/string") << "String does not begin with '\"'.";
			return (false);
		}
		if (*vstr.rbegin() != '"') {
			ERROR("/config/type/string") << "String does not end with '\"'.";
			return (false);
		}

		std::string str(vstr.begin() + 1, vstr.end() - 1);
		if (std::find(str.begin(), str.end(), '"') != str.end()) {
			ERROR("/config/type/string") << "String has '\"' other than at beginning or end.";
			return (false);
		}

		*stringp = str;
		return (true);
	}
};

extern ConfigTypeString config_type_string;

#endif /* !CONFIG_CONFIG_TYPE_STRING_H */
