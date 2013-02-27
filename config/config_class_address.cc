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
#include <config/config_class_address.h>

ConfigClassAddress config_class_address;

bool
ConfigClassAddress::Instance::activate(const ConfigObject *)
{
	switch (family_) {
	case SocketAddressFamilyIP:
	case SocketAddressFamilyIPv4:
	case SocketAddressFamilyIPv6:
		if (path_ != "") {
			ERROR("/config/class/address") << "IP socket has path field set, which is only valid for Unix domain sockets.";
			return (false);
		}
		return (true);

	case SocketAddressFamilyUnix:
		if (host_ != "" || port_ != "") {
			ERROR("/config/class/address") << "Unix domain socket has host and/or port field set, which is only valid for IP sockets.";
			return (false);
		}
		return (true);

	default:
		ERROR("/config/class/address") << "Unsupported address family.";
		return (false);
	}
}
