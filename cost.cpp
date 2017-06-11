#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <stdlib.h>     /* atof */
#include <string>
#include <map>
#include <iostream>

#include "cost.h"

Cost::Cost(string xmlFile) : XmlDoc(xmlFile) 
{
}

Cost::~Cost()
{
}

void Cost::dumpCost()
{
    map<string, float>::iterator iter;

    cout << "SKU" << '\t' << "Cost" << endl;
    for (iter = costMap.begin(); iter != costMap.end(); iter++)
    {
        cout << iter->first << '\t' << iter->second << endl;
    }
}

bool Cost::addCostToMap(string &sku, float cost)
{
    map<string, float>::iterator iter;

    if (costMap.find(sku) == costMap.end())
    {
        costMap[sku] = cost;
    }
    else
    {
        fprintf(stderr, "found duplicate sku[%s] with cost [%f]. Cannot add another entry with cost [%f]\n", sku.c_str(), costMap[sku], cost);
        return false;
    }
    
    return true;
}

bool Cost::parseProduct(xmlNodePtr product)
{
    bool result;
    xmlNodePtr cur;
    string sku, description;
    float cost;
    int count = 0;

    if (!product)
    {
        fprintf(stderr, "Null pointer. No product to parse\n");
        return false;
    }

    cur = cur->xmlChildrenNode;
    while (cur)
    {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"Sku")))
        {
            sku = parseString(cur);
            count++;
        }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"Cost")))
        {
            cost = parseFloat(cur);
            count++;
        }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"Description")))
        {
            description = parseString(cur);
        }
 
        cur = xmlNextElementSibling(cur);
    }

    if (count != 2)
    {
        fprintf(stderr, "failed to parse product. One or more required nodes not found in product. Only %d of 2 required nodes found.\n", count);
        return false;
    }

    result = addCostToMap(sku, cost);
    if (!result)
    {
        fprintf(stderr, "failed to add cost to map. sku [%s], cost [%f]", sku.c_str(), cost);
        return false;
    }

    return true;
}

bool Cost::parseCost()
{
    bool result;
    xmlNodePtr cur = getRootElement();

    if (!cur)
    {
        fprintf(stderr, "empty document or no/invalid document\n");
        return false;
    }

    printf("name of root node: %s\n", cur->name);

    // Cost
    if (xmlStrcmp(cur->name, (const xmlChar *)"Cost"))
    {
        fprintf(stderr,"document of the wrong type, root node != Cost\n");
        return false;
    }

    cur = cur->xmlChildrenNode;
    while (cur)
    {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"Product")))
        {
            result = parseProduct(cur);

            if (!result)
            {
                fprintf(stderr, "failed to parse node [Product]\n");
                return false;
            }
        }

        cur = xmlNextElementSibling(cur);
    }

    return true;
}
