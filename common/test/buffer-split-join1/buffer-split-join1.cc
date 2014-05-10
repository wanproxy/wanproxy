/*
 * Copyright (c) 2011-2013 Juli Mallett. All rights reserved.
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
