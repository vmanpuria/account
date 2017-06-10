#include <string>
using namespace std;

class OrderedSkuInfo {
    public:
        int qty;
        float price;
        float fee;
        float promotion;

        OrderedSkuInfo() : qty(0), price(0.0), fee(0.0), promotion(0.0) { }

        void operator+=(OrderedSkuInfo &orderedSkuInfo)
        {
            qty += orderedSkuInfo.qty;
            price += orderedSkuInfo.price;
            fee += orderedSkuInfo.fee;
            promotion += orderedSkuInfo.promotion;
        }
};

class SettlementReport
{
    private:
        xmlDocPtr doc; 
        map<string, OrderedSkuInfo> orderedSkus;

        string parseString(xmlDocPtr doc, xmlNodePtr cur);
        float parseFloat(xmlDocPtr doc, xmlNodePtr cur);
        int parseInt(xmlDocPtr doc, xmlNodePtr cur);

        xmlNodePtr findNodeInChildren(xmlNodePtr cur, const xmlChar *key);

        bool findAndParseMessageType(xmlNodePtr amazonEnvelope, string &messageType);
        bool findAndParseMessage(xmlNodePtr amazonEnvelope);
        bool findAndParseSettlementReport(xmlNodePtr message);
        bool findAndParseOrderArray(xmlNodePtr settlementReport);
        bool checkAndParseOrderArray(xmlNodePtr order);
        bool parseOrder(xmlNodePtr order);
        bool findAndParseFulfillment(xmlNodePtr order);
        bool findAndParseItemArray(xmlNodePtr fulfillment);
        bool parseItemArray(xmlNodePtr item);
        bool findAndParseSku(xmlNodePtr item, string &sku);
        bool findAndParseQuantity(xmlNodePtr item, int quantity);
        bool findAndParseItemPrice(xmlNodePtr item, float &itemPrice);
        bool findAndParseItemFees(xmlNodePtr item, float &itemFees);
        bool findAndParsePromotion(xmlNodePtr item, float &itemPromotion);

        bool findAndParseComponentArray(xmlNodePtr itemPrice, float &itemPrice);
        bool parseComponentArray(xmlNodePtr component, float &itemPrice);
        bool findAndParseAmount(xmlNodePtr amountParent, float &amount);
        bool findAndParseFeeArray(xmlNodePtr itemFees, float &itemFees);
        bool parseFeeArray(xmlNodePtr fee, float &fees);

        bool addItemFromOrder(string &sku, int qty, float itemPrice, float itemFees, float itemPromotion);
        void dumpItemsFromOrders();

    public:
        SettlementReport(std::string &docName);
        ~SettlementReport();

        bool findAndParseAmazonEnvelope();
};
