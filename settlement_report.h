#ifndef __SETTLEMENT_REPORT_H__
#define __SETTLEMENT_REPORT_H__

#include "xml_doc.h"
#include "cost.h"

class Summary
{
    public:
        // Order
        int orderCount;
        int orderSkuCount;
        float orderAmount;
        float orderTaxAmount;
        float orderCostAmount;

        // Refund
        int refundCount;
        float refundAmount;
        float refundTaxAmount;

        // Other
        int otherCount;
        float otherAmount;

        // Settlement
        float totalAmount;
        string startDate;
        string endDate;

        Summary() : orderCount(0), orderSkuCount(0), orderAmount(0.0), orderTaxAmount(0.0), orderCostAmount(0.0),
                    refundCount(0), refundAmount(0.0), refundTaxAmount(0.0),
                    otherCount(0), otherAmount(0.0), 
                    totalAmount(0.0), startDate(), endDate() { }
		
		void dump()
        {
            float total = orderAmount + orderTaxAmount + refundAmount + refundTaxAmount + otherAmount; 
            float diff = totalAmount - total;

            cout << endl;
            cout << "Order: orders: " << orderCount << " items: " << orderSkuCount << " amount: " << orderAmount << " tax: " << orderTaxAmount << " cost: " << orderCostAmount << endl;
            cout << "Refund: refunds: " << refundCount << " amount: " << refundAmount << " tax: " << refundTaxAmount << endl;
            cout << "Other: others: " << otherCount << " amount: " << otherAmount << endl;
            cout << "Settlement: amount: " << totalAmount << " start: " << startDate << " end: " << endDate << endl;
            cout << "Total (Order + Refund + Other): amount: " << total << " diff: " << diff << endl;
            cout << "Profit: from orders: " << (orderAmount + orderCostAmount) << " cash profit: " << (total + orderCostAmount) << endl;

            cout << endl;
        }
};

class OrderSkuInfo
{
    public:
        int qty;
        float price;
        float fee;
        float promotion;
        float tax;
        float cost;

        OrderSkuInfo() : qty(0), price(0.0), fee(0.0), promotion(0.0), tax(0.0), cost(0.0) { }

        void operator+=(OrderSkuInfo &orderSkuInfo)
        {
            qty += orderSkuInfo.qty;
            price += orderSkuInfo.price;
            fee += orderSkuInfo.fee;
            promotion += orderSkuInfo.promotion;
            tax += orderSkuInfo.tax;
            cost += orderSkuInfo.cost;
        }
};

class RefundSkuInfo
{
    public:
        float price;
        float fee;
        float promotion;
        float tax;

        RefundSkuInfo() : price(0.0), fee(0.0), promotion(0.0), tax(0.0) { }

        void operator+=(RefundSkuInfo &refundSkuInfo)
        {
            price += refundSkuInfo.price;
            fee += refundSkuInfo.fee;
            promotion += refundSkuInfo.promotion;
            tax += refundSkuInfo.tax;
        }
};

class SettlementReport : public XmlDoc
{
    private:
        Cost cost;
        map<string, OrderSkuInfo> orderSkuMap;
        map<string, RefundSkuInfo> refundSkuMap;
        Summary summary;

        xmlNodePtr findNodeInChildren(xmlNodePtr cur, const xmlChar *key, bool optionalFlag = false);

        bool findAndParseMessageType(xmlNodePtr amazonEnvelope, string &messageType);
        bool findAndParseMessage(xmlNodePtr amazonEnvelope);
        bool findAndParseSettlementReport(xmlNodePtr message);
        bool parseSettlementReport(xmlNodePtr settlementReport);
        bool parseSettlementData(xmlNodePtr settlementData);
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
        bool addItemFromRefund(string &sku, float itemPrice, float itemTax, float itemFees, float itemPromotion);

        bool parseRefund(xmlNodePtr refund);
        bool findAndParseFulfillmentInRefund(xmlNodePtr refund);
        bool parseFulfillment(xmlNodePtr fulfillment);
        bool parseAdjustedItem(xmlNodePtr adjustedItem);
        bool parseItemPriceAdjustments(xmlNodePtr itemPriceAdjustments, float &itemPrice, float &itemTax);
        bool parseItemFeeAdjustments(xmlNodePtr itemFeeAdjustments, float &itemFees);
        bool parseItemPromotionAdjustment(xmlNodePtr itemPromotionAdjustment, float &itemPromotion);
        bool parseOtherTransaction(xmlNodePtr otherTransaction);

        bool parseDoc();

    public:
        SettlementReport(string docName, string costFile);
        ~SettlementReport();

        bool parse();
        void dumpItemsFromOrders();
        void dumpItemsFromRefunds();
        void dumpSummary();
};
#endif //__SETTLEMENT_REPORT_H__
