#include <common/buffer.h>
#include <common/test.h>

struct splitjoin_test_vector {
	const char *input_;
	char split_delim_;
	bool include_empty_;
	size_t split_count_;
	const char *join_delim_;
	const char *output_;
};

static const struct splitjoin_test_vector splitjoin_test_vectors[] = {
	{ "/usr/src",		'/',	true,	3,	".",	".usr.src"		},
	{ "/usr/src",		'/',	false,	2,	".",	"usr.src"		},
	{ "../usr/src",		'/',	false,	3,	".",	"...usr.src"		},
	{ "",			'/',	false,	0,	".",	""			},
	{ "!\1@#$\1%^&**()_",	'\1',	false,	3,	" ",	"! @#$ %^&**()_"	},
	{ "Hello, world!",	' ',	false,	2,	"",	"Hello,world!"		},
	{ NULL,			'\0',	false,	0,	NULL,	NULL			},
};

int
main(void)
{
	TestGroup g("/test/buffer/split-join1", "Buffer::split/Buffer::join #1");
	const struct splitjoin_test_vector *sjtv;

	for (sjtv = splitjoin_test_vectors; sjtv->input_ != NULL; ++sjtv) {
		Buffer in(sjtv->input_);
		std::vector<Buffer> vec = in.split(sjtv->split_delim_, sjtv->include_empty_);
		{
			Test _(g, "Expected size");
			if (vec.size() == sjtv->split_count_)
				_.pass();
		}
		Buffer out = Buffer::join(vec, sjtv->join_delim_);
		{
			Test _(g, "Expected joined output");
			if (out.equal(sjtv->output_))
				_.pass();
		}
	}
}
