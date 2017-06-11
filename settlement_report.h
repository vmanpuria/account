#ifndef __SETTLEMENT_REPORT_H__
#define __SETTLEMENT_REPORT_H__
#include <string>
using namespace std;

class OrderSkuInfo
{
    public:
        int qty;
        float price;
        float fee;
        float promotion;
        float tax;

        OrderSkuInfo() : qty(0), price(0.0), fee(0.0), promotion(0.0), tax(0.0) { }

        void operator+=(OrderSkuInfo &orderSkuInfo)
        {
            qty += orderSkuInfo.qty;
            price += orderSkuInfo.price;
            fee += orderSkuInfo.fee;
            promotion += orderSkuInfo.promotion;
            tax += orderSkuInfo.tax;
        }
};

class RefundSkuInfo
{
    public:
        float price;
        float fee;
        float tax;

        RefundSkuInfo() : price(0.0), fee(0.0), tax(0.0) { }

        void operator+=(RefundSkuInfo &refundSkuInfo)
        {
            price += refundSkuInfo.price;
            fee += refundSkuInfo.fee;
            tax += refundSkuInfo.tax;
        }
};

class SettlementReport
{
    private:
        xmlDocPtr doc; 
        map<string, OrderSkuInfo> orderSkuMap;
        map<string, RefundSkuInfo> refundSkuMap;

        string parseString(xmlNodePtr cur);
        float parseFloat(xmlNodePtr cur);
        int parseInt(xmlNodePtr cur);

        xmlNodePtr findNodeInChildren(xmlNodePtr cur, const xmlChar *key, bool optionalFlag = false);

        bool findAndParseMessageType(xmlNodePtr amazonEnvelope, string &messageType);
        bool findAndParseMessage(xmlNodePtr amazonEnvelope);
        bool findAndParseSettlementReport(xmlNodePtr message);
        bool findCheckAndParseOrderArray(xmlNodePtr settlementReport);
        bool checkAndParseOrderArray(xmlNodePtr order);
        bool parseOrder(xmlNodePtr order);
        bool findAndParseFulfillment(xmlNodePtr order);
        bool findAndParseItemArray(xmlNodePtr fulfillment);
        bool parseItemArray(xmlNodePtr item);
        bool findAndParseSku(xmlNodePtr item, string &sku);
        bool findAndParseQuantity(xmlNodePtr item, int &quantity);
        bool findAndParseItemPrice(xmlNodePtr item, float &itemPrice, float &itemTax);
        bool findAndParseItemFees(xmlNodePtr item, float &itemFees);
        bool findAndParsePromotion(xmlNodePtr item, float &itemPromotion);

        bool findAndParseComponentArray(xmlNodePtr itemPrice, float &price, float &tax);
        bool parseComponentArray(xmlNodePtr component, float &itemPrice, float &itemTax);
        bool parseComponent(xmlNodePtr component, float &amount, bool &taxFlag);
        bool findAndParseAmountAndType(xmlNodePtr amountParent, float &amount, string &amountType);
        bool findAndParseFeeArray(xmlNodePtr itemFees, float &fees);
        bool parseFeeArray(xmlNodePtr fee, float &fees);

        bool addItemFromOrder(string &sku, int qty, float itemPrice, float itemTax, float itemFees, float itemPromotion);
        bool addItemFromRefund(string &sku, float itemPrice, float itemTax, float itemFees);

        bool findCheckAndParseRefundArray(xmlNodePtr settlementReport);
        bool checkAndParseRefundArray(xmlNodePtr refund);
        bool parseRefund(xmlNodePtr refund);
        bool findAndParseFulfillmentInRefund(xmlNodePtr refund);
        bool findAndParseAdjustedItem(xmlNodePtr fulfillment);
        bool parseItemPriceAdjustments(xmlNodePtr itemPriceAdjustments, float &itemPrice, float &itemTax);
        bool parseItemFeeAdjustments(xmlNodePtr itemFeeAdjustments, float &itemFees);

    public:
        SettlementReport(std::string docName);
        ~SettlementReport();

        bool findAndParseAmazonEnvelope();
        void dumpItemsFromOrders();
        void dumpItemsFromRefunds();
};
#endif //__SETTLEMENT_REPORT_H__
