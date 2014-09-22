// myxml.h
/*
 *  Copyright (c) 2008 Kelly Schrock
 *
 *  This file is part of SDDM.
 *
 *  SDDM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SDDM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SDDM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _myxml_h
#define _myxml_h

#include <map>
#include <stack>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

using namespace std;

struct IXMLParser {
	virtual ~IXMLParser() {}
	
	
};

struct IXMLListener {
	virtual ~IXMLListener() {}

	virtual void start() = 0;
	virtual void elementStart(string name) = 0;
	virtual void elementEnd() = 0;
	virtual void attribute(string name, string value) = 0;
	virtual void content(string content) = 0;
	virtual void end() = 0;
};

struct IXMLWriter;

/** A reader of XML. */
struct IXMLReader {
	virtual string getData() = 0;

	virtual ~IXMLReader() {}
};

/** A reader that pulls XML from a file. */
class XMLFileReader: public IXMLReader {
public:
	XMLFileReader(const char *filename);
	virtual string getData();

private:
	string filename;
};

/** An XMLReader that supplies a user-specified string. */
class XMLStringReader: public IXMLReader  {
public:
	XMLStringReader(string data): data(data) {}
	~XMLStringReader() {}

	virtual string getData() { return data; }
private:
	string data;
};

/** An XML document. */
class XMLDocument {
public:
	class Exception {
	public:
		Exception(const char *what)
		: reason(what)
		{}

		string getMessage() { return reason; }
	private:
		string reason;
	};

	class Attribute {
	public:
		typedef vector<XMLDocument::Attribute*> List;

		string name, value;

	public:
		Attribute(string name)
		: name(name), value("")
		{}

		Attribute(string name, string value)
		: name(name), value(value)
		{}

		virtual ~Attribute() {}

		string& getName() { return name; }
		string& getValue() { return value; }
	};

	class Element {
	public:
		typedef vector<XMLDocument::Element*> List;

		string name;
		string content;
		XMLDocument::Element::List elements;
		XMLDocument::Attribute::List attributes;

	public:
		Element(string name)
		: name(name)
		{}

		Element(string name, string content)
		: name(name)
		, content(content)
		{}

		virtual ~Element();

		string& getName() { return name; }

		XMLDocument::Element::List& getElements() { return elements; }
		XMLDocument::Element::List getElements(string name)
		{
			XMLDocument::Element::List list;
			
			for(XMLDocument::Element::List::iterator e = getElements().begin(); e != getElements().end(); ++e)
			{
				if((*(e))->getName() == name)
					list.push_back((*(e)));
			}
			
			return list;
		}
		
		XMLDocument::Attribute::List& getAttributes() { return attributes; }
		XMLDocument::Attribute* findAttribute(string name) 
		{
			for(XMLDocument::Attribute::List::iterator e = attributes.begin(); e != attributes.end(); ++e)
				if((*(e))->getName() == name)
					return *e;
			return 0;
		}
		
		XMLDocument::Element* findElement(string& name)
		{
			XMLDocument::Element::List elems = getElements();
			for(XMLDocument::Element::List::iterator e = elems.begin(); e != elems.end(); ++e)
			{
				XMLDocument::Element* elem = *e;
				if(elem->getName() == name)
					return elem;
			}
			return 0;
		}
		
		XMLDocument::Element* findElement(const char* name) 
		{ 
			string s(name);
			return findElement(s); 
		}

		string getAttributeValue(string name)
		{
			XMLDocument::Attribute *a = findAttribute(name);
			return (a)? a->getValue(): string("");
		}
		
		string getAttributeValue(const char *name)
		{
			string s(name);
			return getAttributeValue(s);
		}

		XMLDocument::Element& setContent(string content) { this->content = content; return *this; }
		bool hasContent() { return !content.empty(); }
		string& getContent() { return content; }

		XMLDocument::Element& add(XMLDocument::Element* elem) { elements.push_back(elem); return *this; }
		XMLDocument::Element& add(XMLDocument::Attribute* attr) { attributes.push_back(attr); return *this; }
		XMLDocument::Element& setAttributes(XMLDocument::Attribute::List a) { attributes = a; return *this; }

		XMLDocument::Element& addAttribute(string name, string value)
		{
			attributes.push_back(new XMLDocument::Attribute(name, value));
			return *this;
		}

		XMLDocument::Element& addAttribute(const char* name, const char* value)
		{
			return addAttribute(string(name), string(value));
		}
	};

	XMLDocument()
	: reader(0)
	{}

	XMLDocument(IXMLReader *reader);
	virtual ~XMLDocument();

	virtual bool parse();
	virtual bool write(IXMLWriter *writer);

	virtual XMLDocument& setTopElement(XMLDocument::Element *elem) { topElement = elem; return *this; }
	virtual XMLDocument::Element *getTopElement() { return topElement; }

	static string normalize(string& s);
	static string decode(string& s);

private:
	IXMLReader *reader;
	XMLDocument::Element *topElement;
};

class XMLListener: public IXMLListener {
public:
	typedef std::stack<XMLDocument::Element*> ElementStack;

	XMLListener(XMLDocument* doc)
	: doc(doc)
	{}

	virtual ~XMLListener() {};

	virtual void start();
	virtual void end();
	virtual void elementStart(string name);
	virtual void attribute(string name, string value);
	virtual void elementEnd();
	virtual void content(string content);

private:
	XMLDocument* doc;
	ElementStack elements;
};


/** Writes XML to a destination. */
struct IXMLWriter {
	virtual ~IXMLWriter() {}

	virtual bool start() = 0;
	virtual void elementStart(XMLDocument::Element *) = 0;
	virtual void elementEnd(XMLDocument::Element *) = 0;
	virtual bool end() = 0;
};

/** Writes XML to a string. */
class XMLStringWriter: public IXMLWriter {
private:
	string contents;
	unsigned indent;
	
protected:
	ostringstream stream;
	string tabs(unsigned indent);

public:
	XMLStringWriter()
	: indent(0)
	{}

	virtual ~XMLStringWriter() {}

	string& getContents() { return contents; }
	ostringstream& getStream() { return stream; }

	virtual bool start();
	virtual void elementStart(XMLDocument::Element *);
	virtual void elementEnd(XMLDocument::Element *);
	virtual bool end();
};

/** Writes XML to a file. */
class XMLFileWriter: public XMLStringWriter {
private:
	string filename;

public:
	typedef stack<XMLDocument::Element*> ElementStack;

	XMLFileWriter(const char* filename)
	: XMLStringWriter()
	, filename(filename)
	{}

	virtual ~XMLFileWriter() {}

	string& getFilename() { return filename; }

	virtual bool end();
};


// ostream& operator<<(ostream& out, XMLDocument::Attribute& attr)
// {
// 	out << "operator << not implemented for XMLDocument::Attribute yet";
// 	return out;
// }
//
// ostream& operator<<(ostream& out, XMLDocument::Element& elem)
// {
// 	out << "operator << not implemented for XMLDocument::Element yet";
// 	return out;
// }
//
// ostream& operator<<(ostream& out, XMLDocument& doc)
// {
// 	out << "operator << not implemented for XMLDocument yet";
// 	return out;
// }



#endif // _myxml_h
