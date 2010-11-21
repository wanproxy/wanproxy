#include <common/buffer.h>
#include <common/test.h>

#include <xml/xml.h>

int
main(void)
{
	TestGroup g("/test/xml/serialize1", "XML (serialize) #1");

	Ref<XML::Element> root = new XML::Element("test");

	root->add(new XML::Attribute("version", "\"1.0'"));
	root->add(Buffer("A "));
	root->add(new XML::Element("e", Buffer("world")));
	root->add(Buffer("!"));
	root->add(new XML::Element("x"));

	XML::Document d(root);

	Buffer out;
	d.serialize(&out);

	{
		Test _(g, "Expected contents.");
		if (out.equal("<?xml version=\"1.0\"?><test version=\"&quot;1.0'\">A <e>world</e>!<x/></test>"))
			_.pass();
	}

	return (0);
}
