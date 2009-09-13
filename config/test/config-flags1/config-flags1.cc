#include <common/test.h>

#include <config/config.h>
#include <config/config_class.h>
#include <config/config_object.h>
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

class TestConfigClassFlags : public ConfigClass {
public:
	TestConfigClassFlags(void)
	: ConfigClass("test-config-flags")
	{
		add_member("flags1", &test_config_type_flags);
		add_member("flags2", &test_config_type_flags);
		add_member("flags3", &test_config_type_flags);
		add_member("flags4", &test_config_type_flags);
	}

	~TestConfigClassFlags()
	{ }

	bool activate(ConfigObject *co)
	{
		TestConfigTypeFlags *ct;
		ConfigValue *cv;
		unsigned int flags;

		cv = co->get("flags1", &ct);
		if (cv == NULL) {
			ERROR("/test/config/flags/class") << "Could not get flags1.";
			return (false);
		}

		if (!ct->get(cv, &flags)) {
			ERROR("/test/config/flags/class") << "Could not get flags1.";
			return (false);
		}

		if (flags != FLAG_A) {
			ERROR("/test/config/flags/class") << "Field (flags1) does not have expected value.";
			return (false);
		}

		cv = co->get("flags2", &ct);
		if (cv == NULL) {
			ERROR("/test/config/flags/class") << "Could not get flags2.";
			return (false);
		}

		if (!ct->get(cv, &flags)) {
			ERROR("/test/config/flags/class") << "Could not get flags2.";
			return (false);
		}

		if (flags != (FLAG_A | FLAG_B | FLAG_C)) {
			ERROR("/test/config/flags/class") << "Field (flags2) does not have expected value.";
			return (false);
		}

		cv = co->get("flags3", &ct);
		if (cv == NULL) {
			ERROR("/test/config/flags/class") << "Could not get flags3.";
			return (false);
		}

		if (!ct->get(cv, &flags)) {
			ERROR("/test/config/flags/class") << "Could not get flags3.";
			return (false);
		}

		if (flags != FLAG_C) {
			ERROR("/test/config/flags/class") << "Field (flags3) does not have expected value.";
			return (false);
		}

		cv = co->get("flags4", &ct);
		if (cv != NULL) {
			ERROR("/test/config/flags/class") << "Could get flags4.";
			return (false);
		}

		INFO("/test/config/flags/class") << "Got all expected values.";

		return (true);
	}
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
