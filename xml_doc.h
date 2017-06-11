#ifndef __XML_DOC_H__
#define __XML_DOC_H__

#include <string>
using namespace std;

class XmlDoc
{
    private:
        xmlDocPtr doc; 

    public:
        XmlDoc(string xmlFile);
        ~XmlDoc();

        xmlNodePtr getRootElement();
        string parseString(xmlNodePtr cur);
        float parseFloat(xmlNodePtr cur);
        int parseInt(xmlNodePtr cur);
};
#endif //__XML_DOC_H__
