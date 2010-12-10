#include <common/buffer.h>
#include <common/test.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/dropbox.h>
#include <event/event_main.h>

struct DropboxTest {
	unsigned key_;
	TestGroup& group_;
	Test test_;
	Dropbox<unsigned> dropbox_;
	Action *action_;

	DropboxTest(unsigned key, TestGroup& group)
	: key_(key),
	  group_(group),
	  test_(group_, "Dropbox is used."),
	  dropbox_(),
	  action_(NULL)
	{
		TypedCallback<unsigned> *cb = callback(this, &DropboxTest::item_dropped);
		if (key_ % 2 == 0) {
			action_ = dropbox_.get(cb);
			dropbox_.put(key_);
		} else {
			dropbox_.put(key_);
			action_ = dropbox_.get(cb);
		}
	}

	~DropboxTest()
	{
		{
			Test _(group_, "No outstanding action.");
			if (action_ == NULL)
				_.pass();
		}
	}

	void item_dropped(unsigned key)
	{
		test_.pass();
		{
			Test _(group_, "Outstanding action.");
			if (action_ != NULL) {
				action_->cancel();
				action_ = NULL;

				_.pass();
			}
		}
		{
			Test _(group_, "Correct value.");
			if (key == key_)
				_.pass();
		}

		delete this;
	}
};

int
main(void)
{
	TestGroup g("/test/dropbox1", "Dropbox #1");

	unsigned i;
	for (i = 0; i < 100; i++) {
		new DropboxTest(i, g);
	}

	event_main();
}
