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

#include <config/config_class.h>
#include <config/config_exporter.h>
#include <config/config_type.h>

ConfigClass::~ConfigClass()
{
	std::map<std::string, const ConfigClassMember *>::iterator it;

	while ((it = members_.begin()) != members_.end()) {
		delete it->second;
		members_.erase(it);
	}
}

bool
ConfigClass::set(ConfigObject *co, const std::string& mname, const std::string& vstr) const
{
#if 0
	ASSERT("/config/class", co->class_ == this);
#endif

	std::map<std::string, const ConfigClassMember *>::const_iterator it = members_.find(mname);
	if (it == members_.end()) {
		ERROR("/config/class") << "Object member (" << mname << ") does not exist.";
		return (false);
	}
	return (it->second->set(co, vstr));
}

void
ConfigClass::marshall(ConfigExporter *exp, const ConfigClassInstance *co) const
{
	std::map<std::string, const ConfigClassMember *>::const_iterator it;
	for (it = members_.begin(); it != members_.end(); ++it) {
		exp->field(co, it->second, it->first);
	}
}
