/*
 * Copyright (c) 2010-2011 Juli Mallett. All rights reserved.
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

#ifndef	NETWORK_NETWORK_INTERFACE_PCAP_H
#define	NETWORK_NETWORK_INTERFACE_PCAP_H

struct pcap;

typedef	struct pcap pcap_t;

class NetworkInterfacePCAP : public NetworkInterface {
	LogHandle log_;
	pcap_t *pcap_;

	EventCallback *receive_callback_;
	Action *receive_action_;
private:
	NetworkInterfacePCAP(pcap_t *);

public:
	~NetworkInterfacePCAP();

	Action *receive(EventCallback *);
	Action *transmit(Buffer *, EventCallback *);

private:
	void receive_callback(Event);
	void receive_cancel(void);
	Action *receive_do(void);
	Action *receive_schedule(void);

public:
	static NetworkInterface *open(const std::string&);
};

#endif /* !NETWORK_NETWORK_INTERFACE_PCAP_H */
