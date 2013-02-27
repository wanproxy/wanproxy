/*
 * Copyright (c) 2011 Juli Mallett. All rights reserved.
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

#include <common/buffer.h>
#include <common/test.h>

int
main(void)
{
	TestGroup g("/test/buffer/split1", "Buffer::split #1");

	{
		Test _(g, "No fields");
		Buffer buf("");
		std::vector<Buffer> vec = buf.split(',');
		if (vec.size() == 0)
			_.pass();
	}

	{
		Test _(g, "Empty buffer, include empty fields");
		Buffer buf("");
		std::vector<Buffer> vec = buf.split(',', true);
		if (vec.size() == 1 && vec[0].empty())
			_.pass();
	}

	{
		Test _(g, "Two empty fields");
		Buffer buf(",");
		std::vector<Buffer> vec = buf.split(',');
		if (vec.size() == 0)
			_.pass();
	}

	{
		Test _(g, "Two empty fields, include empty fields");
		Buffer buf(",");
		std::vector<Buffer> vec = buf.split(',', true);
		if (vec.size() == 2 && vec[0].empty() && vec[1].empty())
			_.pass();
	}

	{
		Test _(g, "One field");
		Buffer buf("Hello");
		std::vector<Buffer> vec = buf.split(',');
		if (vec.size() == 1 && vec[0].equal("Hello"))
			_.pass();
	}

	{
		Test _(g, "One field among empties");
		Buffer buf(",Hello,");
		std::vector<Buffer> vec = buf.split(',');
		if (vec.size() == 1 && vec[0].equal("Hello"))
			_.pass();
	}

	{
		Test _(g, "Two fields among empties");
		Buffer buf(",Hello,,,World,");
		std::vector<Buffer> vec = buf.split(',');
		if (vec.size() == 2 && vec[0].equal("Hello") && vec[1].equal("World"))
			_.pass();
	}

	{
		Test _(g, "Two fields and an empty, include empty fields");
		Buffer buf("Hello,World,");
		std::vector<Buffer> vec = buf.split(',', true);
		if (vec.size() == 3 && vec[0].equal("Hello") && vec[1].equal("World") && vec[2].empty())
			_.pass();
	}
}
