#ifndef	XML_XML_H
#define	XML_XML_H

#include <common/ref.h>

#include <list>

namespace XML {
	class Attribute {
		std::string key_;
		std::string value_;

	public:
		Attribute(const std::string& key, const std::string& value)
		: key_(key),
		  value_(value)
		{ }

		~Attribute()
		{ }

		void serialize(Buffer *) const;
	};

	class Element {
		class Content {
			enum Type {
				CharacterDataContentType,
				ElementContentType,
			};

			Type type_;
			Buffer buffer_;
			Ref<Element> element_;

		public:
			Content(const Buffer& buffer)
			: type_(CharacterDataContentType),
			  buffer_(buffer),
			  element_()
			{ }

			Content(const Ref<Element>& element)
			: type_(ElementContentType),
			  buffer_(),
			  element_(element)
			{ }

			~Content()
			{ }

			void serialize(Buffer *) const;
		};

		std::string tag_;
		std::list<Ref<Attribute> > attributes_;
		std::list<Ref<Content> > content_;

	public:
		Element(const std::string& tag)
		: tag_(tag),
		  attributes_(),
		  content_()
		{ }

		Element(const std::string& tag, const Buffer& buffer)
		: tag_(tag),
		  attributes_(),
		  content_()
		{
			add(buffer);
		}

		~Element()
		{ }

		void serialize(Buffer *) const;

		void add(const Ref<Attribute>& attribute)
		{
			attributes_.push_back(attribute);
		}

		void add(const Ref<Element>& element)
		{
			content_.push_back(new Content(element));
		}

		void add(const Buffer& buffer)
		{
			content_.push_back(new Content(buffer));
		}
	};

	class Document {
		Ref<Element> root_;

	public:
		Document(const Ref<Element>& root)
		: root_(root)
		{ }

		~Document()
		{ }

		void serialize(Buffer *) const;
	};

	void escape(Buffer *, const Buffer *);
}

#endif /* !XML_XML_H */
