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
	bool quiet, verbose;
	int ch;

	quiet = false;
	verbose = false;

	INFO("/wanproxy") << "WANProxy";
	INFO("/wanproxy") << "Copyright (c) 2008-2012 WANProxy.org.";
	INFO("/wanproxy") << "All rights reserved.";

	while ((ch = getopt(argc, argv, "c:qv")) != -1) {
		switch (ch) {
		case 'c':
			configfile = optarg;
			break;
		case 'q':
			quiet = true;
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

	if (quiet && verbose)
		usage();

	if (verbose) {
		Log::mask(".?", Log::Debug);
	} else if (quiet) {
		Log::mask(".?", Log::Error);
	} else {
		Log::mask(".?", Log::Info);
	}

	WANProxyConfig config;
	if (!config.configure(configfile)) {
		ERROR("/wanproxy") << "Could not configure proxies.";
		return (1);
	}

	event_main();
}

static void
usage(void)
{
	INFO("/wanproxy/usage") << "wanproxy [-q | -v] -c configfile";
	exit(1);
}
