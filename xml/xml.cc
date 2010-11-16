#include <common/buffer.h>

#include <xml/xml.h>

using namespace XML;

void
Attribute::serialize(Buffer *out) const
{
	out->append(key_);
	out->append("=\"");
	out->append(value_); /* XXX Escape.  */
	out->append("\"");
}

void
Element::Content::serialize(Buffer *out) const
{
	switch (type_) {
	case CharacterDataContentType:
		out->append(buffer_); /* XXX Escape.  */
		break;
	case ElementContentType:
		ASSERT(!element_.null());
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

		ASSERT(!attribute.null());

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
		ASSERT(!content.null());
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

	ASSERT(!root_.null());

	root_->serialize(out);
}
