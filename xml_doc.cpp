#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <stdlib.h>     /* atof */
#include <string>
#include <map>
#include <iostream>

#include "xml_doc.h"

XmlDoc::XmlDoc(string xmlFile)
{
    doc = xmlParseFile(xmlFile.c_str());

    if (doc == NULL)
    {
        fprintf(stderr,"Document [%s] not parsed successfully. \n", xmlFile.c_str());
        return;
    }
}

XmlDoc::~XmlDoc()
{
    if (doc)
    {
        xmlFreeDoc(doc);
        doc = NULL;
    }
}

string XmlDoc::parseString(xmlNodePtr cur)
{
    xmlChar *key;
	string str;

    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
    str = (const char *)key;
    xmlFree(key);

    return str;
}

float XmlDoc::parseFloat(xmlNodePtr cur)
{
	float val;
    string str;

    str = parseString(cur);
    val = atof(str.c_str());

    return val;
}

int XmlDoc::parseInt(xmlNodePtr cur)
{
    int val;
    string str;

    str = parseString(cur);
    val = atoi(str.c_str());

    return val;
}

xmlNodePtr XmlDoc::getRootElement()
{
    if (!doc)
    {
        fprintf(stderr, "No document. failed to get root element\n");
        return NULL;
    }

    return xmlDocGetRootElement(doc);
}
