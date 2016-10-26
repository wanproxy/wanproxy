/*
 * Copyright (c) 2009-2015 Juli Mallett. All rights reserved.
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
#include <config/config_type_size.h>

ConfigTypeSize config_type_size;

void
ConfigTypeSize::marshall(ConfigExporter *exp, const intmax_t *valp) const
{
	static const char *suffixes[] = { "kb", "mb", "gb", "tb", NULL };
	const char *suffix = "b", **suffixp;
	intmax_t val = *valp;

	for (suffixp = suffixes; *suffixp != NULL; suffixp++) {
		if (val < 1024)
			break;
		/* Lose less than a quarter of an increment.  */
		if (val < 1024 * 1024 && val % 1024 > 256)
			break;
		val /= 1024;
		suffix = *suffixp;
	}

	char buf[sizeof val * 3 + 2 + 1];
	snprintf(buf, sizeof buf, "%ju%s", val, suffix);

	std::ostringstream os;
	os << *valp << " (" << buf << ")";

	exp->value(this, os.str());
}

bool
ConfigTypeSize::set(ConfigObject *, const std::string& vstr, intmax_t *valp)
{
	const char *str = vstr.c_str();
	char *endp;
	intmax_t imax;

#if !defined(__OPENNT)
	imax = strtoimax(str, &endp, 0);
#else
	imax = strtoll(str, &endp, 0);
#endif
	switch (tolower(*endp)) {
	case 't':
		imax *= 1024;
		EXPLICIT_FALLTHROUGH;
	case 'g':
		imax *= 1024;
		EXPLICIT_FALLTHROUGH;
	case 'm':
		imax *= 1024;
		EXPLICIT_FALLTHROUGH;
	case 'k':
		imax *= 1024;
		EXPLICIT_FALLTHROUGH;
	case 'b':
		endp++;
		EXPLICIT_FALLTHROUGH;
	case '\0':
		break;
	default:
		ERROR("/config/type/size") << "Invalid size suffix.";
		return (false);
	}

	std::string suffix(endp);
	if (suffix != "B" && suffix != "b" && suffix != "") {
		ERROR("/config/type/size") << "Invalid secondary size suffix.";
		return (false);
	}

	*valp = imax;

	return (true);
}
