/*
 * Copyright (c) 2010-2012 Juli Mallett. All rights reserved.
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

#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#include <network/network_interface.h>

class PacketDumper {
	NetworkInterface *interface_;
	Action *receive_action_;
	Action *stop_action_;
public:
	PacketDumper(NetworkInterface *interface)
	: interface_(interface),
	  receive_action_(NULL)
	{
		EventCallback *cb = callback(this, &PacketDumper::receive_complete);
		receive_action_ = interface_->receive(cb);

		SimpleCallback *scb = callback(this, &PacketDumper::stop);
		stop_action_ = EventSystem::instance()->register_interest(EventInterestStop, scb);
	}

	~PacketDumper()
	{
		if (receive_action_ != NULL) {
			receive_action_->cancel();
			receive_action_ = NULL;
		}
	}

private:
	void receive_complete(Event e)
	{
		receive_action_->cancel();
		receive_action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		default:
			HALT("/packet/dumper") << "Unexpected event: " << e;
			return;
		}

		INFO("/packet/dumper") << e.buffer_.length() << " byte packet: " << std::endl << e.buffer_.hexdump();

		EventCallback *cb = callback(this, &PacketDumper::receive_complete);
		receive_action_ = interface_->receive(cb);
	}

	void stop(void)
	{
		stop_action_->cancel();
		stop_action_ = NULL;

		ASSERT("/packet/dumper", receive_action_ != NULL);
		receive_action_->cancel();
		receive_action_ = NULL;
	}
};

static void usage(void);

int
main(int argc, char *argv[])
{
	const char *ifname;
	int ch;

	ifname = NULL;

	while ((ch = getopt(argc, argv, "i:")) != -1) {
		switch (ch) {
		case 'i':
			if (ifname != NULL)
				usage();
			ifname = optarg;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (ifname == NULL)
		usage();

	if (argc != 0)
		usage();

	NetworkInterface *interface = NetworkInterface::open(ifname);
	if (interface == NULL) {
		ERROR("/main") << "Unable to open interface.";
		return (1);
	}

	PacketDumper dumper(interface);

	event_main();

	delete interface;
}

static void
usage(void)
{
	ERROR("/main") << "usage: network-packet-receive-hexdump1 -i ifname";
	exit(1);
}
