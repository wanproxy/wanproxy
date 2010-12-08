#include <common/buffer.h> /* XXX For class Event */

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

class ReloadTest {
	Action *action_;
	Action *reload_action_;
	unsigned reload_count_;
public:
	ReloadTest(void)
	: action_(NULL),
	  reload_action_(NULL),
	  reload_count_(0)
	{
		action_ = callback(this, &ReloadTest::forever)->schedule();
		reload_action_ = EventSystem::instance()->register_interest(EventInterestReload, callback(this, &ReloadTest::reload));
	}

	~ReloadTest()
	{
		ASSERT(action_ == NULL);
	}

private:
	void forever(void)
	{
		action_->cancel();
		action_ = NULL;

		action_ = callback(this, &ReloadTest::forever)->schedule();
	}

	void reload(void)
	{
		reload_action_->cancel();
		reload_action_ = NULL;

		INFO("/event/reload/test1") << "Reload #" << ++reload_count_;

		reload_action_ = EventSystem::instance()->register_interest(EventInterestReload, callback(this, &ReloadTest::reload));
	}
};

int
main(void)
{
	ReloadTest *tt = new ReloadTest();
	EventSystem::instance()->start();
	delete tt;
}
