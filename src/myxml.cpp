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


//
// myxml.cpp
// C++ XML parsers piss me off. This one is designed not to.
//

#include <iostream>
#include <fstream>
#include <stack>

using namespace std;

#include "myxml.h"

static string trim(string str, string delims = " \t\r\n")
{
	string::size_type pos1 = str.find_first_not_of(delims);
	string::size_type pos2 = str.find_last_not_of(delims);

	if(pos1 == string::npos)
		pos1 = 0;

	if(pos2 == string::npos)
		pos2 = str.length()-1;

	str = str.substr(pos1, pos2-(pos1-1));
	return str;
}

static vector<string> tokenize(string s, string delims = " ")
{
	vector<string> tokens;

	string::size_type lastPos = s.find_first_not_of(delims, 0);
	string::size_type pos = s.find_first_of(delims, lastPos);

	while(string::npos != pos || string::npos != lastPos)
	{
  	tokens.push_back(s.substr(lastPos, pos - lastPos));
   	lastPos = s.find_first_not_of(delims, pos);
  	pos = s.find_first_of(delims, lastPos);
	}

	return tokens;
}

static void stripTopCrap(string& src)
{
	string::size_type start = src.find("<?");
	if(start != string::npos)
	{
		string::size_type end = src.find("?>", start);
		while(end != string::npos && end > start)
		{
			src.erase(src.begin() + start, src.begin() + end+2);
			start = src.find("<?");
			end = src.find("?>", start);
		}
	}
}

static void stripComments(string& src)
{
	string::size_type start = src.find("<!--");
	if(start != string::npos)
	{
		string::size_type end = src.find("-->", start);

		while(end != string::npos && end > start)
		{
			src.erase(src.begin() + start, src.begin() + end+3);
			start = src.find("<!--");
			end = src.find("-->", start);
		}
	}
}

//
// ----------------XMLListener
//
void XMLListener::start()
{}

void XMLListener::end()
{}

void XMLListener::elementStart(string name)
{
	elements.push(new XMLDocument::Element(name));
}

void XMLListener::elementEnd()
{
	if(!elements.empty())
	{
		XMLDocument::Element* elem = elements.top();
		elements.pop();
		if(elements.empty())
			doc->setTopElement(elem);
		else
			elements.top()->add(elem);
	}
}

void XMLListener::attribute(string name, string value)
{
	if(!elements.empty())
		elements.top()->add(new XMLDocument::Attribute(name, XMLDocument::decode(value)));
}

void XMLListener::content(string content)
{
	if(!elements.empty())
		elements.top()->setContent(XMLDocument::decode(content));
}

//
// -----------------XMLFileReader
//
/**
 * Construct an XMLFileReader for the specified filename
 * @param filename
 */
XMLFileReader::XMLFileReader(const char *filename)
: filename(filename)
{}

string XMLFileReader::getData()
{
	string data;

	ifstream in(filename.c_str());

	if(in && in.is_open())
	{
		while(!in.eof())
		{
			string line;
			getline(in, line);
			data += trim(line);
		}

		in.close();
	}

	return data;
}

//
// -------------------------XMLDocument
//
XMLDocument::~XMLDocument()
{
	delete topElement;
}

XMLDocument::XMLDocument(IXMLReader *reader)
: reader(reader)
{}

string extractName(string s)
{
	string name;

	string::size_type term = s.find_first_of(" \t\r\n>");

	for(string::size_type i = 0; i < term; ++i)
	{
		if(s[i] == '<' || s[i] == '>')
			continue;

		if(s[i] == ' ')
			break;
		name += s[i];
	}

	return name;
}

string XMLDocument::normalize(string& str)
{
	string s;

	for(string::iterator e = str.begin(); e != str.end(); ++e)
	{
		char ch = *e;

		switch(ch)
		{
			case '<':
			{
				s += "&lt;";
				break;
			}
			case '>':
			{
				s += "&gt;";
				break;
			}
			
			case '&':
			{
				s += "&amp;";
				break;
			}
			
			case '"':
			{
				s += "&quot;";
				break;
			}
			
			default:
			{
				s += ch;
				break;
			}
		}
	}
	
	return s;
}

string XMLDocument::decode(string& str)
{
	string ret(str);
	
	map<string,char> tags;

	tags["&lt;"] = '<';
	tags["&gt;"] = '>';
	tags["&amp;"] = '&';
	tags["&quot;"] = '"';

	for(map<string,char>::iterator e = tags.begin(); e != tags.end(); ++e)
	{
		string key = (*(e)).first;
		char val = (*(e)).second;
		
		const char cz[] = {val, 0};
		
		string::size_type start = ret.find(key);
		while(start != string::npos)
		{
			ret.replace(start, key.length(), cz);
			start = ret.find(key);
		}
	}
		
	return ret;
}


string extractBlock(string& src)
{
	string block;

	string::size_type start = src.find("<");

	if(start != string::npos)
	{
		string::size_type end = src.find(">", start);

		bool simple = src[end-1] == '/';

		if(simple)
		{
			block = trim(src.substr(start, end));
			
			if(block[block.length()-1] != '>')
				block += '>';
		}
		else
		{
			if(end != string::npos)
			{
				string name = extractName(src.substr(start+1, end-start));
				string startName = "<" + name;
				string endName = "</" + name + ">";

				string::size_type st = src.find(startName);
				string::size_type ex = src.find(endName);

				ex += endName.length();
				block = src.substr(st, ex-st);
			}
		}
	}

	return block;
}

XMLDocument::Attribute::List extractAttributes(string& name, string& src)
{
	XMLDocument::Attribute::List list;

	string::size_type start = src.find(name);
	string::size_type end = src.find(">", start+1);
	
	bool simple = (src[end-1] == '/');
	
	string::size_type len = name.length();

	if(simple)
		--end;
		
	string s = trim(src.substr(start+len, (end-(start+len))));
	
	vector<string> tokens = tokenize(s, "\"");
	unsigned i = 0;
	for(vector<string>::iterator e = tokens.begin(); e != tokens.end(); ++e, ++i)
	{
		string token((*(e)));
		
		if(token.find("=") == string::npos)
		{
			if(i > 0)
			{
				// Get the name from the previous element
				string name(*(e-1));
				vector<string> parts = tokenize(name, "=");
				list.push_back(new XMLDocument::Attribute(trim(parts[0]), token));
			}
		}
	}
	
	return list;
}

string extractContent(string& src)
{
	string::size_type contentStart = src.find(">");
	string::size_type contentEnd = src.find("<", contentStart);

	if(contentStart != string::npos && contentEnd != string::npos)
		return trim(src.substr(++contentStart, --contentEnd-contentStart));

	return string();
}

string read(string& src, IXMLListener* listener)
{
	string block = extractBlock(src);
	if(block.empty())
		return block;

	//cout << "block=" << block << endl;

	string name = extractName(block);

	listener->elementStart(name);

	string endElemName = "</" + name + ">";

	string content = extractContent(block);

	if(!content.empty())
		listener->content(content);

	string::size_type elemEnd = (block.find("/>") != string::npos)?
		block.length():
		block.find(endElemName)+endElemName.length();

//	cout << "name=" << name << endl;

	XMLDocument::Attribute::List attrs = extractAttributes(name, block);
	for(XMLDocument::Attribute::List::iterator e = attrs.begin(); e != attrs.end(); ++e)
		listener->attribute((*(e))->getName(), (*(e))->getValue());

	// Find the next sub-element
	string::size_type first = block.find("<");
	string::size_type more = block.find("<", first+name.length());

	if(more != string::npos)
	{
		string next = block.substr(more, (elemEnd-more));
		next = trim(next);

		// Collect blocks between here and the end of the nested elements.
		// TODO: This is D_I_R_T_Y. Need to marklar the Marklar.
		vector<string> blocks;
		while(true)
		{
			string sub = extractBlock(next);
			//cout << "sub=" << sub << endl;

			if(sub.empty())
				break;

			more += sub.length();

			if(more > (elemEnd-endElemName.length()))
				break;

			blocks.push_back(sub);
			next = block.substr(more, (elemEnd-more));
		}

		for(vector<string>::iterator e = blocks.begin(); e != blocks.end(); ++e)
		{
			string b(*(e));
			read(b, listener);
		}
	}

	listener->elementEnd();

	return block;
}

bool writeOut(XMLDocument::Element *elem, IXMLWriter* writer)
{
	writer->elementStart(elem);

	XMLDocument::Element::List elements = elem->getElements();
	for(XMLDocument::Element::List::iterator e = elements.begin(); e != elements.end(); ++e)
		writeOut((*(e)), writer);

	writer->elementEnd(elem);
	return true;
}

bool XMLDocument::write(IXMLWriter *writer)
{
	if(!writer)
		return false;

	writer->start();

	XMLDocument::Element *top = getTopElement();
	if(!top)
		return false;

	writeOut(top, writer);
	writer->end();
	return true;
}

static string tabs(unsigned indent)
{
	string s;
	for(unsigned i = 0; i < indent; ++i)
		s += "\t";
	return s;
}

void dump(XMLDocument::Element *elem, unsigned indent)
{
 	cout << tabs(indent) << "element: " << elem->getName() << endl;
	
	XMLDocument::Attribute::List attrs = elem->getAttributes();
	for(XMLDocument::Attribute::List::iterator e = attrs.begin(); e != attrs.end(); ++e)
		cout << tabs(indent+1) << "attribute: " << (*(e))->getName() << "=" << (*(e))->getValue() << endl;
		
	if(elem->hasContent())
		cout << tabs(indent) << "content: " << elem->getContent() << endl;
		
	XMLDocument::Element::List elems = elem->getElements();
	for(XMLDocument::Element::List::iterator e = elems.begin(); e != elems.end(); ++e)
		dump((*(e)), indent+1);		
}

bool XMLDocument::parse()
{
	if(!reader)
		return false;

	string data = reader->getData();
//	cout << "data=" << data << endl;

	IXMLListener *reader = new XMLListener(this);
	reader->start();

	stripComments(data);
	stripTopCrap(data);
	read(data, reader);

	reader->end();
	
	bool result = (topElement != 0);
	
/*	if(topElement)
		dump(topElement, 0);*/
	
	delete reader;
	return result;
}

//
// --------------------------XMLStringWriter
//
string XMLStringWriter::tabs(unsigned indent)
{
	string s;
	for(unsigned i = 0; i < indent; ++i)
		s += "\t";
	return s;
}

bool XMLStringWriter::start()
{
	stream
		<< "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	return true;
}

void XMLStringWriter::elementStart(XMLDocument::Element *elem)
{
	stream
		<< tabs(indent)
		<< "<" << elem->getName();

	XMLDocument::Attribute::List list = elem->getAttributes();
	for(XMLDocument::Attribute::List::iterator e = list.begin(); e != list.end(); ++e)
	{
		XMLDocument::Attribute* a(*(e));

		stream
		<< " "
		<< a->getName() << "=\"" << XMLDocument::normalize(a->getValue()) << "\"";
	}

	stream << ">" << endl;

	if(elem->hasContent())
		stream << tabs(indent+1) << XMLDocument::normalize(elem->getContent()) << endl;

	++indent;
}

void XMLStringWriter::elementEnd(XMLDocument::Element *elem)
{
	--indent;
	stream	<< tabs(indent)	<< "</" << elem->getName() << ">" << endl;
}

bool XMLStringWriter::end()
{
	contents = stream.str();
	return true;
}

//
// --------------------------XMLFileWriter
//
bool XMLFileWriter::end()
{
	XMLStringWriter::end();

	ofstream out(filename.c_str());
	if(!out)
		return false;

    out << stream.str() << endl;

	out.close();
	return true;
}

//
// -------------------------XMLDocument::Element
//
XMLDocument::Element::~Element()
{
	for(Attribute::List::iterator e = attributes.begin(); e != attributes.end(); ++e)
		delete *e;
		
	for(Element::List::iterator e = elements.begin(); e != elements.end(); ++e)
		delete *e;		
}

