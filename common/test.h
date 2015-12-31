/*
 * Copyright (c) 2008-2013 Juli Mallett. All rights reserved.
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

#ifndef	COMMON_TEST_H
#define	COMMON_TEST_H

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
		ASSERT_NON_ZERO(log_, tests_);

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

#endif /* !COMMON_TEST_H */
