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

#include <http/http_protocol.h>

namespace {
	static bool decode_nibble(uint8_t *out, uint8_t ch)
	{
		if (ch >= '0' && ch <= '9') {
			*out |= 0x00 + (ch - '0');
			return (true);
		}
		if (ch >= 'a' && ch <= 'f') {
			*out |= 0x0a + (ch - 'a');
			return (true);
		}
		if (ch >= 'A' && ch <= 'F') {
			*out |= 0x0a + (ch - 'A');
			return (true);
		}
		return (false);
	}
}

bool
HTTPProtocol::Message::decode(Buffer *input)
{
	if (start_line_.empty()) {
		start_line_.clear();
		headers_.clear();
		body_.clear();
	}

	Buffer line;
	if (ExtractLine(&line, input) != ParseSuccess) {
		ERROR("/http/protocol/message") << "Could not get start line.";
		return (false);
	}
	if (line.empty()) {
		ERROR("/http/protocol/message") << "Premature end of headers.";
		return (false);
	}
	start_line_ = line;

	if (type_ == Request) {
		/*
		 * There are two kinds of request line.  The first has two
		 * words, the second has three.  Anything else is malformed.
		 *
		 * The first kind is HTTP/0.9.  The second kind can be
		 * anything, especially HTTP/1.0 and HTTP/1.1.
		 */
		std::vector<Buffer> words = line.split(' ', false);
		if (words.empty()) {
			ERROR("/http/protocol/message") << "Empty start line.";
			return (false);
		}
		if (words.size() == 2) {
			/*
			 * HTTP/0.9.  This is all we should get from the client.
			 */
			return (true);
		}

		if (words.size() != 3) {
			ERROR("/http/protocol/message") << "Too many request parameters.";
			return (false);
		}
	} else {
		ASSERT("/http/protocol/message", type_ == Response);
		/*
		 * No parsing behavior is conditional for responses.
		 */
		line.clear();
	}

	/*
	 * HTTP/1.0 or HTTP/1.1.  Get headers.
	 */
	std::string last_header;
	for (;;) {
		ASSERT("/http/protocol/message", line.empty());
		if (ExtractLine(&line, input) != ParseSuccess) {
			ERROR("/http/protocol/message") << "Could not extract line for headers.";
			return (false);
		}

		/*
		 * Process end of headers!
		 */
		if (line.empty()) {
			/*
			 * XXX
			 * Use Content-Length, Transfer-Encoding, etc.
			 */
			if (!input->empty())
				input->moveout(&body_);
			return (true);
		}

		/*
		 * Process header.
		 */
		if (line.peek() == ' ') { /* XXX isspace? */
			/*
			 * Fold headers per RFC822.
			 * 
			 * XXX Always forget how to handle leading whitespace.
			 */
			if (last_header == "") {
				ERROR("/http/protocol/message") << "Folded header sent before any others.";
				return (false);
			}

			line.moveout(&headers_[last_header].back());
			continue;
		}

		size_t pos;
		if (!line.find(':', &pos)) {
			ERROR("/http/protocol/message") << "Empty header name.";
			return (false);
		}

		Buffer key;
		line.moveout(&key, pos);
		line.skip(1);

		Buffer value;
		while (!line.empty() && line.peek() == ' ')
			line.skip(1);
		value = line;

		std::string header;
		key.extract(header);

		headers_[header].push_back(value);
		last_header = header;

		line.clear();
	}
}

bool
HTTPProtocol::DecodeURI(Buffer *encoded, Buffer *decoded)
{
	if (encoded->empty())
		return (true);

	for (;;) {
		size_t pos;
		if (!encoded->find('%', &pos)) {
			encoded->moveout(decoded);
			return (true);
		}
		if (pos != 0)
			encoded->moveout(decoded, pos);
		if (encoded->length() < 3)
			return (false);
		uint8_t vis[2];
		encoded->copyout(vis, 1, 2);
		uint8_t byte = 0x00;
		if (!decode_nibble(&byte, vis[0]))
			return (false);
		byte <<= 4;
		if (!decode_nibble(&byte, vis[1]))
			return (false);
		decoded->append(byte);
		encoded->skip(3);
	}
}

HTTPProtocol::ParseStatus
HTTPProtocol::ExtractLine(Buffer *line, Buffer *input, Buffer *line_ending)
{
	ASSERT("/http/protocol/extract/line", line->empty());

	if (input->empty()) {
		DEBUG("/http/protocol/extract/line") << "Empty buffer.";
		return (ParseIncomplete);
	}

	size_t pos;
	uint8_t found;
	if (!input->find_any("\r\n", &pos, &found)) {
		DEBUG("/http/protocol/extract/line") << "Incomplete line.";
		return (ParseIncomplete);
	}

	/*
	 * XXX
	 * We should pick line ending from the start line and require it to
	 * be consistent for remaining lines, rather than using find_any over
	 * and over, which is non-trivial.  Handling of the start line can be
	 * quite easily and cheaply before the loop.
	 */
	switch (found) {
	case '\r':
		/* CRLF line endings.  */
		ASSERT("/http/protocol/extract/line", input->length() > pos);
		if (input->length() == pos + 1) {
			DEBUG("/http/protocol/extract/line") << "Carriage return at end of buffer, need following line feed.";
			return (ParseIncomplete);
		}
		if (pos != 0)
			input->moveout(line, pos);
		input->skip(1);
		if (input->peek() != '\n') {
			ERROR("/http/protocol/extract/line") << "Carriage return not followed by line feed.";
			return (ParseFailure);
		}
		input->skip(1);
		if (line_ending != NULL)
			line_ending->append("\r\n");
		break;
	case '\n':
		/* Unix line endings.  */
		if (pos != 0)
			input->moveout(line, pos);
		input->skip(1);
		if (line_ending != NULL)
			line_ending->append("\n");
		break;
	default:
		NOTREACHED("/http/protocol/extract/line");
	}

	return (ParseSuccess);
}
