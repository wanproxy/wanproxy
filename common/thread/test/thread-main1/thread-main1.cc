#include <common/test.h>

#include <common/thread/thread.h>

#define	NTHREAD		8
#define	ROUNDS		1024

class TestThread : public Thread {
	Test *test_main_;
	Test *test_destroy_;
public:
	TestThread(Test *test_main, Test *test_destroy)
	: Thread("TestThread"),
	  test_main_(test_main),
	  test_destroy_(test_destroy)
	{ }

	~TestThread()
	{
		test_destroy_->pass();
	}

	void main(void)
	{
		test_main_->pass();
	}
};

int
main(void)
{
	Thread *threads[NTHREAD];
	Test *test_main[NTHREAD], *test_destroy[NTHREAD];

	TestGroup g("/test/thread/main1", "Thread::main #1");

	unsigned j;
	for (j = 0; j < ROUNDS; j++) {
		unsigned i;
		for (i = 0; i < NTHREAD; i++) {
			test_main[i] = new Test(g, "Main function called.");
			test_destroy[i] = new Test(g, "Destructor called.");
			threads[i] = new TestThread(test_main[i], test_destroy[i]);
		}

		for (i = 0; i < NTHREAD; i++)
			threads[i]->start();

		for (i = 0; i < NTHREAD; i++)
			threads[i]->join();

		for (i = 0; i < NTHREAD; i++)
			delete test_main[i];

		for (i = 0; i < NTHREAD; i++)
			delete threads[i];

		for (i = 0; i < NTHREAD; i++)
			delete test_destroy[i];
	}

	return (0);
}
