#ifndef __COST_H__
#define __COST_H__

#include "xml_doc.h"

class Cost: public XmlDoc
{
    private:
        map<string, float> costMap;
        bool parseProduct(xmlNodePtr product);
        bool addCostToMap(string &sku, float cost);

    public:
        Cost(string xmlFile);
        ~Cost();
        bool parseCostReport();
        void dumpCost();

        bool getCost(string &sku, float &cost);
};
#endif //__COST_H__
