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

#ifndef	CONFIG_CONFIG_TYPE_FLAGS_H
#define	CONFIG_CONFIG_TYPE_FLAGS_H

#include <map>

#include <config/config_exporter.h>
#include <config/config_type.h>

template<typename T>
class ConfigTypeFlags : public ConfigType {
public:
	struct Mapping {
		const char *string_;
		T flag_;
	};
private:
	std::map<std::string, T> flag_map_;
public:
	ConfigTypeFlags(const std::string& xname, struct Mapping *mappings)
	: ConfigType(xname),
	  flag_map_()
	{
		ASSERT("/config/type/flags", mappings != NULL);
		while (mappings->string_ != NULL) {
			flag_map_[mappings->string_] = mappings->flag_;
			mappings++;
		}
	}

	~ConfigTypeFlags()
	{ }

	void marshall(ConfigExporter *exp, const T *flagsp) const
	{
		T flags = *flagsp;

		std::string str;
		typename std::map<std::string, T>::const_iterator it;
		for (it = flag_map_.begin(); it != flag_map_.end(); ++it) {
			if ((it->second & flags) == 0)
				continue;
			flags &= ~it->second;
			if (!str.empty())
				str += "|";
			str += it->first;
		}
		if (flags != 0)
			HALT("/config/type/flags") << "Trying to marshall unknown flags.";
		if (flags == 0 && str.empty())
			str = "0";
		exp->value(this, str);
	}

	bool set(ConfigObject *, const std::string& vstr, T *flagsp)
	{
		if (flag_map_.find(vstr) == flag_map_.end()) {
			ERROR("/config/type/flags") << "Invalid value (" << vstr << ")";
			return (false);
		}

		*flagsp |= flag_map_[vstr];

		return (true);
	}
};

#endif /* !CONFIG_CONFIG_TYPE_FLAGS_H */
