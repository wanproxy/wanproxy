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

#if !defined(__OPENNT)
#include <inttypes.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include <config/config_exporter.h>
#include <config/config_type_int.h>

ConfigTypeInt config_type_int;

void
ConfigTypeInt::marshall(ConfigExporter *exp, const intmax_t *valp) const
{
	intmax_t val = *valp;

	char buf[sizeof val * 2 + 2 + 1];
	snprintf(buf, sizeof buf, "0x%016jx", val);
	exp->value(this, buf);
}

bool
ConfigTypeInt::set(ConfigObject *, const std::string& vstr, intmax_t *valp)
{
	const char *str = vstr.c_str();
	char *endp;
	intmax_t imax;

#if !defined(__OPENNT)
	imax = strtoimax(str, &endp, 0);
#else
	imax = strtoll(str, &endp, 0);
#endif
	if (*endp != '\0') {
		ERROR("/config/type/int") << "Invalid numeric format.";
		return (false);
	}

	*valp = imax;

	return (true);
}
