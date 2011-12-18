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
		switch (tmp.peek()) {
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
		tmp.skip(1);
	}
	if (!tmp.empty())
		out->append(tmp);
}
