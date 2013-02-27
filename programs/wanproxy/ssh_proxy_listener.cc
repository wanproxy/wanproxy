/*
 * Copyright (c) 2008-2012 Juli Mallett. All rights reserved.
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

#include <common/endian.h>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/socket/socket.h>

#include <io/net/tcp_server.h>

#include "ssh_proxy_connector.h"
#include "ssh_proxy_listener.h"

#include "wanproxy_codec_pipe_pair.h"

SSHProxyListener::SSHProxyListener(const std::string& name,
				   WANProxyCodec *interface_codec,
				   WANProxyCodec *remote_codec,
				   SocketAddressFamily interface_family,
				   const std::string& interface,
				   SocketAddressFamily remote_family,
				   const std::string& remote_name)
: SimpleServer<TCPServer>("/wanproxy/proxy/" + name + "/listener", interface_family, interface),
  name_(name),
  interface_codec_(interface_codec),
  remote_codec_(remote_codec),
  remote_family_(remote_family),
  remote_name_(remote_name)
{ }

SSHProxyListener::~SSHProxyListener()
{ }

void
SSHProxyListener::client_connected(Socket *socket)
{
	PipePair *pipe_pair = new WANProxyCodecPipePair(interface_codec_, remote_codec_);
	new SSHProxyConnector(name_, pipe_pair, socket, remote_family_, remote_name_, interface_codec_, remote_codec_);
}
