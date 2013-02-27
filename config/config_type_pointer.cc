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

#include <stdio.h>

#include <config/config.h>
#include <config/config_class.h>
#include <config/config_exporter.h>
#include <config/config_object.h>
#include <config/config_type_pointer.h>

ConfigTypePointer config_type_pointer;

void
ConfigTypePointer::marshall(ConfigExporter *exp, ConfigObject *const *cop) const
{
	const ConfigObject *co = *cop;

	if (co == NULL)
		exp->value(this, "None");
	else
		exp->value(this, co->name_);
}

bool
ConfigTypePointer::set(ConfigObject *co, const std::string& vstr, ConfigObject **cop)
{
	if (vstr == "None") {
		*cop = NULL;
		return (true);
	}

	ConfigObject *target = co->config_->lookup(vstr);
	if (target == NULL) {
		ERROR("/config/type/pointer") << "Referenced object (" << vstr << ") does not exist.";
		return (false);
	}

	*cop = target;

	return (true);
}
