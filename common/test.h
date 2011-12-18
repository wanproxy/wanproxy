#ifndef	TEST_H
#define	TEST_H

class TestGroup {
	friend class Test;

	LogHandle log_;
	const std::string description_;
	unsigned tests_;
	unsigned passes_;
public:
	TestGroup(const LogHandle& log, const std::string& description)
	: log_(log),
	  description_(description),
	  tests_(0),
	  passes_(0)
	{
		INFO(log_) << "Running tests in group: " << description_;
	}

	~TestGroup()
	{
		ASSERT(log_, tests_ != 0);

		INFO(log_) << "Test results for group: " << description_;
		if (passes_ == tests_) {
			INFO(log_) << "All tests passed.";
		} else {
			if (passes_ == 0) {
				ERROR(log_) << "All tests failed.";
			} else {
				INFO(log_) << passes_ << "/" << tests_ << " tests passed.";
				ERROR(log_) << (tests_ - passes_) << " tests failed.";
			}
		}
	}

private:
	void test(bool passed, const std::string& description)
	{
		if (passed) {
#if 0
			DEBUG(log_) << "PASS: " << description;
#endif
			passes_++;
		} else {
			ERROR(log_) << "FAIL: " << description;
		}
		tests_++;
	}
};

class Test {
	TestGroup& group_;
	const std::string description_;
	bool passed_;
public:
	Test(TestGroup& group, const std::string& description)
	: group_(group),
	  description_(description),
	  passed_(false)
	{ }

	Test(TestGroup& group, const std::string& description, bool passed)
	: group_(group),
	  description_(description),
	  passed_(passed)
	{ }

	~Test()
	{
		group_.test(passed_, description_);
	}

	void pass(void)
	{
		ASSERT("/test", !passed_);
		passed_ = true;
	}
};

#endif /* !TEST_H */
