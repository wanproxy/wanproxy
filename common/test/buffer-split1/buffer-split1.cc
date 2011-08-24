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
