/*
 * Copyright (c) 2014-2016 Juli Mallett. All rights reserved.
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

#ifndef	IRC_IRC_COMMAND_H
#define	IRC_IRC_COMMAND_H

namespace IRC {
	struct Command {
		Buffer source_;
		Buffer command_;
		std::vector<Buffer> params_;
		bool last_param_;

		Command(void)
		: source_(),
		  command_(),
		  params_(),
		  last_param_(false)
		{ }

		Command(const std::string& command)
		: source_(),
		  command_(command),
		  params_(),
		  last_param_(false)
		{ }

		Command& param(const std::string& s)
		{
			return param(Buffer(s));
		}

		Command& param(const Buffer& p)
		{
			ASSERT("/irc/command", !last_param_);

			size_t o;
			if (p.find(' ', &o)) {
				last_param_ = true;
				Buffer quoted(":");
				quoted.append(p);
				params_.push_back(quoted);
				return (*this);
			}

			params_.push_back(p);
			return (*this);
		}

		bool parse(Buffer *in)
		{
			source_.clear();
			command_.clear();
			params_.clear();
			last_param_ = false;

			if (in->empty())
				return (false);
			if (in->peek() == ':') {
				in->skip(1);
				size_t o;
				if (!in->find(' ', &o))
					return (false);
				in->moveout(&source_, o);
				in->skip(1);
			}

			size_t o;
			if (!in->find(' ', &o)) {
				in->moveout(&command_);
			} else {
				in->moveout(&command_, o);
				in->skip(1);
			}
			command_ = command_.toupper();

			while (!in->empty()) {
				Buffer p;
				if (in->peek() == ':') {
					last_param_ = true;
					in->moveout(&p);
				} else {
					if (!in->find(' ', &o)) {
						in->moveout(&p);
					} else {
						in->moveout(&p, o);
						in->skip(1);
					}
				}
				params_.push_back(p);
			}

			return (true);
		}

		void serialize(Buffer *out) const
		{
			if (!source_.empty()) {
				out->append(source_);
				out->append(" ");
			}
			ASSERT("/irc/command", !command_.empty());
			out->append(command_.toupper());
			if (!params_.empty()) {
				out->append(" ");
				out->append(Buffer::join(params_, " "));
			}
		}
	};
}

#endif /* !IRC_IRC_COMMAND_H */
