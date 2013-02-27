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

#include <common/test.h>

#include <config/config.h>
#include <config/config_class.h>
#include <config/config_type_flags.h>

#define	FLAG_A	(0x00000001)
#define	FLAG_B	(0x00000002)
#define	FLAG_C	(0x00000004)

typedef ConfigTypeFlags<unsigned int> TestConfigTypeFlags;

static struct TestConfigTypeFlags::Mapping test_config_type_flags_map[] = {
	{ "A",		FLAG_A },
	{ "B",		FLAG_B },
	{ "C",		FLAG_C },
	{ NULL,		0 }
};

static TestConfigTypeFlags
	test_config_type_flags("test-flags", test_config_type_flags_map);

#if 0
	template<typename T, typename Ti>
	int foo(T Ti::*f);
#endif

class TestConfigClassFlags : public ConfigClass {
	struct Instance : public ConfigClassInstance {
		unsigned int flags1_;
		unsigned int flags2_;
		unsigned int flags3_;
		unsigned int flags4_;

		Instance(void)
		: flags1_(0),
		  flags2_(0),
		  flags3_(0),
		  flags4_(0)
		{ }

		bool activate(const ConfigObject *)
		{
			if (flags1_ != FLAG_A) {
				ERROR("/test/config/flags/class") << "Field (flags1) does not have expected value.";
				return (false);
			}

			if (flags2_ != (FLAG_A | FLAG_B | FLAG_C)) {
				ERROR("/test/config/flags/class") << "Field (flags2) does not have expected value.";
				return (false);
			}

			if (flags3_ != FLAG_C) {
				ERROR("/test/config/flags/class") << "Field (flags3) does not have expected value.";
				return (false);
			}

			if (flags4_ != 0) {
				ERROR("/test/config/flags/class") << "Field (flags4) does not have expected value.";
				return (false);
			}

			INFO("/test/config/flags/class") << "Got all expected values.";

			return (true);
		}

	};
public:
	TestConfigClassFlags(void)
	: ConfigClass("test-config-flags", new ConstructorFactory<ConfigClassInstance, Instance>)
	{
		add_member("flags1", &test_config_type_flags, &Instance::flags1_);
		add_member("flags2", &test_config_type_flags, &Instance::flags2_);
		add_member("flags3", &test_config_type_flags, &Instance::flags3_);
		add_member("flags4", &test_config_type_flags, &Instance::flags4_);
	}

	~TestConfigClassFlags()
	{ }
};

int
main(void)
{
	Config config;
	TestConfigClassFlags test_config_class_flags;

	config.import(&test_config_class_flags);

	TestGroup g("/test/config/flags1", "ConfigTypeFlags #1");
	{
		Test _(g, "Create test object.");

		if (config.create("test-config-flags", "test"))
			_.pass();
	}
	{
		Test _(g, "Fail to create duplicate test object.");

		if (!config.create("test-config-flags", "test"))
			_.pass();
	}
	{
		Test _(g, "Set flags1.");
		if (config.set("test", "flags1", "A"))
			_.pass();
	}
	{
		Test _(g, "Set flags2.");
		if (config.set("test", "flags2", "B"))
			_.pass();
	}
	{
		Test _(g, "Premature activate test object.");
		if (!config.activate("test"))
			_.pass();
	}
	{
		Test _(g, "Set flags3.");
		if (config.set("test", "flags3", "C"))
			_.pass();
	}
	{
		Test _(g, "Premature activate test object again.");
		if (!config.activate("test"))
			_.pass();
	}
	{
		Test _(g, "Correctly-set flags2.  (Including duplicate B.)");
		if (config.set("test", "flags2", "A") &&
		    config.set("test", "flags2", "B") &&
		    config.set("test", "flags2", "C"))
			_.pass();
	}
	{
		Test _(g, "Activate test object.");
		if (config.activate("test"))
			_.pass();
	}
}
