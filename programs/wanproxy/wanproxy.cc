#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <xcodec/xcdb.h>
#include <xcodec/xcodec.h>

#include "wanproxy_config.h"

static void usage(void);

int
main(int argc, char *argv[])
{
	std::string configfile("");
	int ch;

	INFO("/wanproxy") << "WANProxy";
	INFO("/wanproxy") << "Copyright (c) 2008 WANProxy.org.";
	INFO("/wanproxy") << "All rights reserved.";

	while ((ch = getopt(argc, argv, "c:")) != -1) {
		switch (ch) {
		case 'c':
			configfile = optarg;
			break;
		default:
			usage();
		}
	}

	if (configfile == "")
		usage();

	XCDatabase xcdb("/wanproxy/memory");
	XCodec xc("/wanproxy", &xcdb);

	WANProxyConfig config;
	if (!config.configure(&xc, configfile))
		HALT("/wanproxy") << "Could not configure proxies.";
	EventSystem::instance()->start();
}

static void
usage(void)
{
	INFO("/wanproxy/usage") << "wanproxy -c configfile";
	exit(1);
}
