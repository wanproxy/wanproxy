/*
 * Copyright (c) 2010-2012 Juli Mallett. All rights reserved.
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

#include <xml/xml.h>

using namespace XML;

void
Attribute::serialize(Buffer *out) const
{
	out->append(key_);

	out->append("=\"");

	Buffer tmp(value_);
	XML::escape(out, &tmp);

	out->append("\"");
}

void
Element::Content::serialize(Buffer *out) const
{
	switch (type_) {
	case CharacterDataContentType:
		XML::escape(out, &buffer_);
		break;
	case ElementContentType:
		ASSERT("/xml/element/content", !element_.null());
		element_->serialize(out);
		break;
	}
}

void
Element::serialize(Buffer *out) const
{
	out->append("<");
	out->append(tag_);

	std::list<Ref<Attribute> >::const_iterator ait;
	for (ait = attributes_.begin(); ait != attributes_.end(); ++ait) {
		Ref<Attribute> attribute = *ait;

		ASSERT("/xml/element", !attribute.null());

		out->append(" ");
		attribute->serialize(out);
	}

	if (content_.empty()) {
		out->append("/>");
		return;
	}

	out->append(">");

	std::list<Ref<Content> >::const_iterator cit;
	for (cit = content_.begin(); cit != content_.end(); ++cit) {
		Ref<Content> content = *cit;
		ASSERT("/xml/element", !content.null());
		content->serialize(out);
	}

	out->append("</");
	out->append(tag_);
	out->append(">");
}

void
Document::serialize(Buffer *out) const
{
	out->append("<?xml version=\"1.0\"?>");

	/* XXX Encoding, doctype, etc.  */

	ASSERT("/xml/document", !root_.null());

	root_->serialize(out);
}

void
XML::escape(Buffer *out, const Buffer *in)
{
	Buffer tmp;
	tmp.append(in);

	unsigned pos;
	while (tmp.find_any("\"<>&", &pos)) {
		if (pos != 0)
			tmp.moveout(out, pos);
		switch (tmp.pop()) {
		case '"':
			out->append("&quot;");
			break;
		case '<':
			out->append("&lt;");
			break;
		case '>':
			out->append("&gt;");
			break;
		case '&':
			out->append("&amp;");
			break;
		default:
			NOTREACHED("/xml");
		}
	}
	if (!tmp.empty())
		out->append(tmp);
}
