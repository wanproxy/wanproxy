/*
 * Copyright (c) 2010 Juli Mallett. All rights reserved.
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
