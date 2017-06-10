#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <stdlib.h>     /* atof */
#include <string>
#include <map>
#include <iostream>

// TODO: check for failure to add to map
bool SettlementReport::addItemFromOrder(string &sku, int qty, float itemPrice, float itemFees, float itemPromotion)
{
    OrderedSkuInfo orderedSkuInfo;
    map<string, OrderedSkuInfo>::iterator iter;

    orderedSkuInfo.qty = qty;
    orderedSkuInfo.price = itemPrice;
    orderedSkuInfo.fee = itemFees;
    orderedSkuInfo.promotion = itemPromotion;

    iter = orderedSkus.find(sku);
    if (iter == orderedSkus.end())
    {
        orderedSkus[sku] = orderedSkuInfo;
    }
    else
    {
        orderedSkus[sku] += orderedSkuInfo;
    }

    return true;
}

xmlNodePtr SettlementReport::findNodeInChildren(xmlNodePtr cur, const xmlChar *key)
{
    if (!cur)
    {
        fprintf(stderr,"cannot find node in children. null node.\n");
        return NULL;
    }

    if (!key)
    {
        fprintf(stderr,"cannot find node in children. null key.\n");
        return NULL;
    }

    // key
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, key))) {
            break;
        }
        cur = xmlNextElementSibling(cur);
    }

    if (!cur)
    {
        fprintf(stderr,"node [%s] not found in children\n", key);
    }

    return cur;
}

string SettlementReport::parseString(xmlDocPtr doc, xmlNodePtr cur)
{
    xmlChar *key;
	string str;

    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
    str = (const char *)key;
    xmlFree(key);

    return str;
}

float SettlementReport::parseFloat(xmlDocPtr doc, xmlNodePtr cur)
{
	float val;
    string str;

    str = parseString(doc, cur);
    val = atof(str.c_str());

    return val;
}

int SettlementReport::parseInt(xmlDocPtr doc, xmlNodePtr cur)
{
    int val;
    string str;

    str = parseString(doc, cur);
    val = atoi(str.c_str());

    return val;
}

bool SettlementReport::parseOrder(xmlNodePtr order)
{
    bool result;

    if (!order)
    {
        fprintf(stderr, "Null pointer. No order node. Cannot parse order.\n");
        return false;
    }

    result = findAndParseFulfillment(order);

    if (!result)
    {
        fprintf(stderr, "failed to find and parse fulfillment in order.\n");
        return false;
    }

    return true;
}

bool SettlementReport::findAndParseFulfillment(xmlNodePtr order)
{
    xmlNodePtr cur;
    bool result;

    if (!order)
    {
        fprintf(stderr, "Null pointer. No order node. Cannot find and parse fulfillment.\n");
        return NULL;
    }

    cur = findNodeInChildren(order, (const xmlChar *)"Fulfillment");

    if (!cur)
    {
        fprintf(stderr,"failed to find node [Fulfillment] in order.\n");
        return false;
    }

    result = findAndParseItemArray(cur);

    if (!result)
    {
        fprintf(stderr, "failed to find and parse item in fulfillment.\n");
        return false;
    }

    return true;
}

bool SettlementReport::findAndParseItemArray(xmlNodePtr fulfillment)
{
    xmlNodePtr cur;
    bool result;

    if (!fulfillment)
    {
        fprintf(stderr, "Null pointer. No fulfillment node. Cannot find and parse item.\n");
        return false;
    }

    cur = findNodeInChildren(fulfillment, (const xmlChar *)"Item");

    if (!cur)
    {
        fprintf(stderr, "failed to find node [Item] in fulfillment\n");
        return false;
    }

    result = parseItemArray(cur);

    if (!result)
    {
        fprintf(stderr, "failed to parse item array in fulfillment\n");
        return false;
    }

    return true;
}

bool SettlementReport::parseFeeArray(xmlNodePtr fee, float &fees)
{
    float amount;

    if (!fees)
    {
        fprintf(stderr,"Null pointer. No Fee node. Cannot parse fee array.\n");
        return false;
    }

    fees = 0.0;
    while (fee)
    {
        result = findAndParseAmount(fee, amount);

        if (!result)
        {
            fprintf(stderr, "failed to find and parse amount in fee.\n");
            return false;
        }

        fees += amount;
        fee = xmlNextElementSibling(fee);
    }

    return true;
}

bool SettlementReport::findAndParseItemPrice(xmlNodePtr item, float &itemPrice)
{
    xmlNodePtr cur;
    bool result;

    if (!item)
    {
        fprintf(stderr, "Null pointer. No item node. Cannot find and parse item price.\n");
        return false;
    }

    cur =  findNodeInChildren(item, (const xmlChar *)"ItemPrice");
    if (!cur)
    {
        fprintf(stderr, "failed to find node [ItemPrice] in item.\n");
        return false;
    }

    result = findAndParseComponentArray(cur, itemPrice);

    if (!result)
    {
        fprintf(stderr, "failed to find and parse component array in node [ItemPrice].\n");
        return false;
    }

    return true;
}

bool SettlementReport::findAndParseComponentArray(xmlNodePtr itemPrice, float &itemPrice)
{
    xmlNodePtr cur;
    bool result;

    if (!itemPrice)
    {
        fprintf(stderr, "Null pointer. No itemPrice node. Cannot find and parse component array.\n");
        return false;
    }

    cur = findNodeInChildren(itemPrice, (const xmlChar *)"Component");

    if (!cur)
    {
        fprintf(stderr, "failed to find node [Component] in itemPrice.\n");
        return false;
    }

    result = parseComponentArray(cur, itemPrice);
    if (!result)
    {
        fprintf(stderr, "failed to parse component array in itemPrice.\n");
        return false;
    }

    return true;
}

bool SettlementReport::parseComponentArray(xmlNodePtr component, float &itemPrice)
{
    float amount;

    if (!component)
    {
        fprintf(stderr, "Null pointer. No component node. Cannot parse component array.\n");
        return false;
    }

    itemPrice = 0.0;
    while (component)
    {
        result = findAndParseAmount(component, amount);
        if (!result)
        {
            fprintf(stderr, "failed to find and parse amount in component.\n");
            return false;
        }

        itemPrice += amount;
        component = xmlNextElementSibling(component);
    }

    return true;
}

bool SettlementReport::findAndParseAmount(xmlNodePtr amountParent, float &amount)
{
    xmlNodePtr cur;

    if (!amountParent)
    {
        fprintf(stderr, "Null pointer. No parent node for amount. Cannot find and parse amount.\n");
        return false;
    }

    cur =  findNodeInChildren(amountParent, (const xmlChar *)"Amount");
    if (!cur)
    {
        fprintf(stderr,"cannot find node [Amount] in parent.\n");
        return false;
    }

    amount = parseFloat(doc, cur);
    return true;
}

bool SettlementReport::findAndParseFeeArray(xmlNodePtr itemFees, float &itemFees)
{
    xmlNodePtr cur;
    bool result;

    if (!itemFees)
    {
        fprintf(stderr,"Null pointer. No itemFees node. Cannot find and parse fees array.\n");
        return false;
    }

    cur = findNodeInChildren(itemFees, (const xmlChar *)"Fee");

    if (!cur)
    {
        fprintf(stderr,"node [Fee] not found in itemFees.\n");
        return false;
    }

    result = parseFeeArray(cur, itemFees);

    if (!result)
    {
        fprintf(stderr,"failed to parse fee array in node [Fee].\n");
        return false;
    }

    return true;
}

bool SettlementReport::findAndParseItemFees(xmlNodePtr item, float &itemFees)
{
    xmlNodePtr cur;

    if (!item)
    {
        fprintf(stderr,"Null pointer. No item node. Cannot find and parse item fees.\n");
        return false;
    }

    //ItemFees
    cur =  findNodeInChildren(item, (const xmlChar *)"ItemFees");

    if (!cur)
    {
        fprintf(stderr,"failed to find node [ItemFees] in item.\n");
        return false;
    }

    result = findAndParseFeeArray(cur, itemFees);
    if (!result)
    {
        fprintf(stderr,"failed to find and parse fee array in node [ItemFees].\n");
        return false;
    }

    return true;
}

bool SettlementReport::findAndParsePromotion(xmlNodePtr item, float &itemPromotion)
{
    xmlNodePtr cur;
    bool result;
    float amount;

    if (!item)
    {
        fprintf(stderr,"Null pointer. No item node. Cannot find and parse promotion.\n");
        return false;
    }

    cur =  findNodeInChildren(item, (const xmlChar *)"Promotion");
    if (!cur)
    {
        fprintf(stderr, "failed to find node [Promotion] in item.\n");
        return false;
    }

    result = findAndParseAmount(cur, amount);

    if (!result)
    {
        fprintf(stderr, "failed to find and parse amount in node [Promotion].\n");
        return false;
    }

    itemPromotion = amount;
    return true;
}

bool SettlementReport::findAndParseSku(xmlNodePtr item, string &sku)
{
    xmlNodePtr cur;

    if (!item)
    {
        fprintf(stderr,"Null pointer. No item node. Cannot find and parse SKU.\n");
        return false;
    }

    cur =  findNodeInChildren(item, (const xmlChar *)"SKU");
    if (!cur)
    {
        fprintf(stderr,"failed to find node [SKU] in item.\n");
        return false;
    }

    sku = parseString(doc, cur);
    return true;
}
 
bool SettlementReport::findAndParseQuantity(xmlNodePtr item, int quantity)
{
    xmlNodePtr cur;

    if (!item)
    {
        fprintf(stderr,"Null pointer. No item node. Cannot find and parse Quantity.\n");
        return false;
    }

    cur =  findNodeInChildren(item, (const xmlChar *)"Quantity");
    if (!cur)
    {
        fprintf(stderr, "failed to find node [Quantity] in item.\n");
        return false;
    }

    quantity = parseInt(doc, cur);
    return true;
}

bool SettlementReport::parseItemArray(xmlNodePtr item)
{
    xmlNodePtr cur;
    bool result;

    string sku;
    int qty;
    float itemPrice, itemFees, itemPromotion;

    if (!item)
    {
        fprintf(stderr, "Null pointer. No item node. Cannot parse item array.\n");
        return false;
    }

	while (item)
    {
        //SKU
        result = findAndParseSku(item, sku);
        if (!result)
        {
            fprintf(stderr, "failt to find and parse sku in item.\n");
            return false;
        }

        printf("sku: %s\n", sku.c_str());

        //Quantity
        result = findAndParseQuantity(item, qty);
        if (!result)
        {
            fprintf(stderr, "failt to find and parse quantity in item.\n");
            return false;
        }

        printf("qty: %d\n", qty);

        //ItemPrice
        result = findAndParseItemPrice(item, itemPrice);
        if (!result)
        {
            fprintf(stderr,"failed to find and parse item price in item.\n");
            return false;
        }
        printf("price: %f\n", itemPrice);

        //ItemFees 
        result = findAndParseItemFees(item, itemFees);

        if (!result)
        {
            fprintf(stderr,"failed to find and parse item fees in item.\n");
            return false;
        }
        printf("itemFees: %f\n", itemFees);

        //Promotion (optional)
        result = findAndParsePromotion(item, itemPromotion);
        if (!result)
        {
            fprintf(stderr,"failed to find and parse promotion in item.\n");
            return false;
        }
 
        result = addItemFromOrder(sku, qty, itemPrice, itemFees, itemPromotion);

        if (!result)
        {
            fprintf(stderr, "failed to add order.\n");
            return false;
        }

        item = xmlNextElementSibling(item);
    }

    //fprintf(stderr,"***** debug ******. return false.\n");
    //return false;
    return true;
}

void SettlementReport::dumpItemsFromOrders()
{
    OrderedSkuInfo orderedSkuInfo;
    int totalQty = 0;
    float revenue, totalRevenue = 0.0;
    map<string, OrderedSkuInfo>::iterator iter = orderedSkus.begin();

    if (iter == orderedSkus.end())
    {
        cout << "No orders" << endl;
        return;
    }
    
    cout << "SKU" << '\t' << "Quantity" << '\t' << "Price" << '\t' << "Fees" << '\t' << '\t' << "Promotion" << "Total Revenue"<< endl;
    for(iter = orderedSkus.begin(); iter != orderedSkus.end(); iter++)
    {
        orderedSkuInfo = iter->second;
        revenue = orderedSkuInfo.price + orderedSkuInfo.fee + orderedSkuInfo.promotion;
        cout << iter->first << '\t' << orderedSkuInfo.qty << '\t' << orderedSkuInfo.price << '\t' << orderedSkuInfo.fee << '\t' << orderedSkuInfo.promotion << '\t' << revenue << endl;

        totalQty += orderedSkuInfo.qty;
        totalRevenue += revenue;
    }

    cout << "Total quantity: " << totalQty << endl;
    cout << "Total Revenue: " << totalRevenue << endl;
}

SettlementReport::SettlementReport(string &docName) : doc(NULL)
{
    doc = xmlParseFile(docName.c_str());

    if (doc == NULL) {
        fprintf(stderr,"Document not parsed successfully. \n");
        return;
    }
}

SettlementReport::~SettlementReport()
{
    if (doc)
    {
        xmlFreeDoc(doc);
        doc = NULL;
    }
}

bool SettlementReport::parseSettlementReport()
{
    bool result;
    xmlNodePtr settlementReport;

    settlementReport = findSettlementReportInDoc();
    result = (settlementReport ? findAndParseOrderArray(settlementReport) : false);
    result = (result ? dumpOrders() : result);
}

bool SettlementReport::findAndParseAmazonEnvelope()
{
    bool result;
    xmlNodePtr cur;
    string messageType;

    if (!doc)
    {
        fprintf(stderr, "No document. failed to find and parse settlement report\n");
        return false;
    }

    cur = xmlDocGetRootElement(doc);

    if (!cur)
    {
        fprintf(stderr, "empty document\n");
        return false;
    }

    printf("name of root node: %s\n", cur->name);

    // AmazonEnvelope
    if (xmlStrcmp(cur->name, (const xmlChar *) "AmazonEnvelope"))
    {
        fprintf(stderr,"document of the wrong type, root node != AmazonEnvelope\n");
        return false;
    }

    result = findAndParseMessageType(cur, messageType);

    if (!result)
    {
        fprintf(stderr, "failed to find and parse messageType in node [AmazonEnvelope]\n");
        return false;
    }

    if ((xmlStrcmp(messageType.c_str(), (const xmlChar *)"SettlementReport")))
    {
        fprintf(stderr, "messageType [%s] is not SettlementReport. Invalid document\n", messageType.c_str());
        return false;
    }

    result = findAndParseMessage(cur);

    if (!result)
    {
        fprintf(stderr, "failed to find and parse message in node [AmazonEnvelope]\n");
        return false;
    }

    return true;
}

bool SettlementReport::findAndParseMessageType(xmlNodePtr amazonEnvelope, string &messageType)
{
    xmlNodePtr cur;

    if (!amazonEnvelope)
    {
        fprintf(stderr, "Null pointer. No amazonEnvelope node. failed to find and parse message type\n");
        return false;
    }

    cur =  findNodeInChildren(amazonEnvelope, (const xmlChar *)"MessageType");
    if (!cur)
    {
        fprintf(stderr, "failed to find node [MessageType] in amazonEnvelope.\n");
        return false;
    }

    messageType = parseString(doc, cur);
    return true;
}

bool SettlementReport::findAndParseMessage(xmlNodePtr amazonEnvelope)
{
    bool result;
    xmlNodePtr cur;

    if (!amazonEnvelope)
    {
        fprintf(stderr, "Null pointer. No amazonEnvelope node. failed to find and parse message\n");
        return false;
    }

    cur =  findNodeInChildren(amazonEnvelope, (const xmlChar *)"Message");
    if (!cur)
    {
        fprintf(stderr, "failed to find node [Message] in amazonEnvelope.\n");
        return false;
    }

    result = findAndParseSettlementReport(cur);

    if (!result)
    {
        fprintf(stderr, "failed to find and parse settlement report in node [Message].\n");
        return false;
    }

    return true;
}

bool SettlementReport::findAndParseSettlementReport(xmlNodePtr message)
{
    bool result;
    xmlNodePtr cur;

    if (!message)
    {
        fprintf(stderr, "Null pointer. No message node. failed to find and parse settlement report\n");
        return false;
    }

    cur =  findNodeInChildren(message, (const xmlChar *)"SettlementReport");
    if (!cur)
    {
        fprintf(stderr, "failed to find node [SettlementReport] in message.\n");
        return false;
    }

    result = findAndParseOrderArray(cur);

    if (!result)
    {
        fprintf(stderr, "failed to find and parse order array in node [SettlementReport].\n");
        return false;
    }

    return true;
}

bool SettlementReport::findAndParseOrderArray(xmlNodePtr settlementReport)
{
    xmlNodePtr cur;
    bool result;

    if (!settlementReport)
    {
        fprintf(stderr, "Null pointer. No settlementReport node. Cannot find and parse order array\n");
        return false;
    }

    cur =  findNodeInChildren(settlementReport, (const xmlChar *)"Order");
    if (!cur)
    {
        fprintf(stderr, "failed to find node [Order] in settlementReport.\n");
        return false;
    }

    result = checkAndParseOrderArray(cur);

    if (!result)
    {
        fprintf(stderr, "failed to check and parse order array in settlementReport.\n");
        return false;
    }

    return true;
}

bool SettlementReport::checkAndParseOrderArray(xmlNodePtr order)
{
    if (!order)
    {
        fprintf(stderr, "Null pointer. No order node. Cannot check and parse order array.\n");
        return false;
    }

    while (order)
    {
        if (!(xmlStrcmp(order->name, (const xmlChar *)"Order")))
        {
            if (!parseOrder(order))
            {
                fprintf(stderr, "failed to parse order in order array.\n");
                break;
            }
        }
        order = xmlNextElementSibling(order);
    }

    return true;
}

int main(int argc, char **argv)
{
    bool result;
    SettlementReport report("./xmlData/5251691949017313.txt");
    printf("\n");
#if 0
    if (argc <= 1) {
        printf("Usage: %s docname\n", argv[0]);
        return(0);
    }
    docname = argv[1];
#endif

    result = report.findAndParseAmazonEnvelope();
    if (!result)
    {
        fprintf(stderr, "failed to parse Amazon envelope.\n");
    }
    return 0;
}
