#include <common/test.h>

#include <common/thread/thread.h>

class TestThread : public Thread {
	Test& test_main_;
	Test& test_destroy_;
public:
	TestThread(Test& test_main, Test& test_destroy)
	: Thread("TestThread"),
	  test_main_(test_main),
	  test_destroy_(test_destroy)
	{ }

	~TestThread()
	{
		test_destroy_.pass();
	}

	void main(void)
	{
		test_main_.pass();
	}
};

int
main(void)
{
	TestGroup g("/test/thread/main1", "Thread::main #1");

	Test test_main(g, "Main function called.");
	Test test_destroy(g, "Destructor called.");
	Thread *td = new TestThread(test_main, test_destroy);

	td->start();

	td->join();

	delete td;

	return (0);
}
