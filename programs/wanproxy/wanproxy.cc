#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_main.h>

#include "wanproxy_config.h"

static void usage(void);

int
main(int argc, char *argv[])
{
	std::string configfile("");
	bool verbose;
	int ch;

	INFO("/wanproxy") << "WANProxy";
	INFO("/wanproxy") << "Copyright (c) 2008-2010 WANProxy.org.";
	INFO("/wanproxy") << "All rights reserved.";

	while ((ch = getopt(argc, argv, "c:v")) != -1) {
		switch (ch) {
		case 'c':
			configfile = optarg;
			break;
		case 'v':
			verbose = true;
			break;
		default:
			usage();
		}
	}

	if (configfile == "")
		usage();

	if (verbose) {
		Log::mask(".?", Log::Debug);
	} else {
		Log::mask(".?", Log::Info);
	}

	WANProxyConfig config;
	if (!config.configure(configfile))
		HALT("/wanproxy") << "Could not configure proxies.";

	event_main();
}

static void
usage(void)
{
	INFO("/wanproxy/usage") << "wanproxy [-v] -c configfile";
	exit(1);
}
