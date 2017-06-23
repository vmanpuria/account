#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <stdlib.h>     /* atof */
#include <string>
#include <map>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <boost/version.hpp>

#include "settlement_report.h"
#include "cost.h"

#undef DEBUG
//#define DEBUG

bool SettlementReport::addItemFromOrder(string &sku, int qty, float itemPrice, float itemTax, float itemFees, float itemPromotion)
{
    OrderSkuInfo orderSkuInfo;
    map<string, OrderSkuInfo>::iterator iter;
    float unitCost, itemCost;
    bool result;

    result = cost.getCost(sku, unitCost);
    if (!result)
    {
        fprintf(stderr, "failed to get unit cost for sku [%s]\n", sku.c_str());
        return false;
    }
    unitCost = -unitCost;
    itemCost = qty * unitCost;

    orderSkuInfo.qty = qty;
    orderSkuInfo.price = itemPrice;
    orderSkuInfo.tax = itemTax;
    orderSkuInfo.fee = itemFees;
    orderSkuInfo.promotion = itemPromotion;
    orderSkuInfo.cost = itemCost;

    iter = orderSkuMap.find(sku);
    if (iter == orderSkuMap.end())
    {
        orderSkuMap[sku] = orderSkuInfo;
    }
    else
    {
        orderSkuMap[sku] += orderSkuInfo;
    }

#ifdef DEBUG
    //printf("sku: %s qty: %d price: %f tax: %f itemFees: %f itemPromotion: %f itemCost: %f\n", sku.c_str(), qty, itemPrice, itemTax, itemFees, itemPromotion, itemCost);
#endif
    summary.orderCount++;
    summary.orderSkuCount += qty;
    summary.orderAmount += itemPrice + itemFees + itemPromotion;
    summary.orderTaxAmount += itemTax;
    summary.orderCostAmount += itemCost;
    return true;
}

bool SettlementReport::addItemFromRefund(string &sku, float itemPrice, float itemTax, float itemFees, float itemPromotion)
{
    RefundSkuInfo refundSkuInfo;
    map<string, RefundSkuInfo>::iterator iter;

    refundSkuInfo.price = itemPrice;
    refundSkuInfo.tax = itemTax;
    refundSkuInfo.fee = itemFees;
    refundSkuInfo.promotion = itemPromotion;

    iter = refundSkuMap.find(sku);
    if (iter == refundSkuMap.end())
    {
        refundSkuMap[sku] = refundSkuInfo;
    }
    else
    {
        refundSkuMap[sku] += refundSkuInfo;
    }

#ifdef DEBUG
    printf("refund: sku: %s price: %f tax: %f itemFees: %f itemPromotion: %f\n", sku.c_str(), itemPrice, itemTax, itemFees, itemPromotion);
#endif
    summary.refundCount++;
    summary.refundAmount += itemPrice + itemFees + itemPromotion;
    summary.refundTaxAmount += itemTax;
    return true;
}

xmlNodePtr SettlementReport::findNodeInChildren(xmlNodePtr cur, const xmlChar *key, bool optionalFlag)
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
        if (!optionalFlag)
        {
            fprintf(stderr,"node [%s] not found in children\n", key);
        }
    }

    return cur;
}

bool SettlementReport::parseSettlementData(xmlNodePtr settlementData)
{
    xmlNodePtr cur;
    int parameterCount = 0;

    if (!settlementData)
    {
        fprintf(stderr, "Null pointer. No settlementData node. Cannot parse.\n");
        return false;
    }

    cur = settlementData->xmlChildrenNode;
    while (cur)
    {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"TotalAmount"))
        {
            summary.totalAmount = parseFloat(cur);
            parameterCount++;
        }
        else if (!xmlStrcmp(cur->name, (const xmlChar *)"StartDate"))
        {
            summary.startDate = parseString(cur);
            parameterCount++;
        }
        else if (!xmlStrcmp(cur->name, (const xmlChar *)"EndDate"))
        {
            summary.endDate = parseString(cur);
            parameterCount++;
        }

        cur = xmlNextElementSibling(cur);
    }

    if (parameterCount != 3)
    {
        fprintf(stderr, "failed to find all 3 parameters in settlementData. found only %d parameters\n", parameterCount);
        return false;
    }

    return true;
}

bool SettlementReport::parseOtherTransaction(xmlNodePtr otherTransaction)
{
    bool result;
    xmlNodePtr cur;
    string transactionType;
    float amount;
    bool typeFlag = false, amountFlag = false;

    if (!otherTransaction)
    {
        fprintf(stderr, "Null pointer. No otherTransaction node. Cannot parse.\n");
        return false;
    }

    cur = otherTransaction->xmlChildrenNode;
    while (cur)
    {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"TransactionType"))
        {
            if (typeFlag)
            {
                fprintf(stderr, "Transaction type was already found. Unexpected.\n");
                return false;
            }

            typeFlag = true;
            transactionType = parseString(cur);
        }
        else if (!xmlStrcmp(cur->name, (const xmlChar *)"Amount"))
        {
            if (amountFlag)
            {
                fprintf(stderr, "Amount was already found. Unexpected.\n");
                return false;
            }

            amountFlag = true;
            amount = parseFloat(cur);
        }

        cur = xmlNextElementSibling(cur);
    }

    if (!typeFlag)
    {
        fprintf(stderr, "Transaction type not found. cannot parse other transaction.\n");
        return false;
    }

    if (!amountFlag)
    {
        fprintf(stderr, "Amount not found. cannot parse other transaction.\n");
        return false;
    }

    if (!transactionType.compare("Inbound Transportation Charge"))
    {
        summary.otherInboundCount++;
        summary.otherInboundAmount += amount;
    }
    else
    {
        summary.otherCount++;
        summary.otherAmount += amount;
    }

    return true;
}

bool SettlementReport::parseRefund(xmlNodePtr refund)
{
    bool result;

    if (!refund)
    {
        fprintf(stderr, "Null pointer. No refund node. Cannot refund order.\n");
        return false;
    }

    result = findAndParseFulfillmentInRefund(refund);

    if (!result)
    {
        fprintf(stderr, "failed to find and parse fulfillment in order.\n");
        return false;
    }

    return true;
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

bool SettlementReport::findAndParseFulfillmentInRefund(xmlNodePtr refund)
{
    xmlNodePtr cur;
    bool result;

    if (!refund)
    {
        fprintf(stderr, "Null pointer. No refund node. Cannot find and parse fulfillment.\n");
        return false;
    }

    cur = findNodeInChildren(refund, (const xmlChar *)"Fulfillment");

    if (!cur)
    {
        fprintf(stderr,"failed to find node [Fulfillment] in refund.\n");
        return false;
    }

    result = parseFulfillment(cur);

    if (!result)
    {
        fprintf(stderr, "failed to find and parse adjusted item in fulfillment.\n");
        return false;
    }

    return true;
}

bool SettlementReport::parseItemPriceAdjustments(xmlNodePtr itemPriceAdjustments, float &itemPrice, float &itemTax)
{
    xmlNodePtr cur;
    bool result;
    float price, tax;

    if (!itemPriceAdjustments)
    {
        fprintf(stderr, "Null pointer. No itemPriceAdjustments node. Cannot parse.\n");
        return false;
    }

    cur = findNodeInChildren(itemPriceAdjustments, (const xmlChar *)"Component");

    if (!cur)
    {
        fprintf(stderr,"failed to find node [Component] in itemPriceAdjustments.\n");
        return false;
    }

    result = parseComponentArray(cur, price, tax);

    if (!result)
    {
        fprintf(stderr, "failed to parse component array in itemPriceAdjustments\n");
        return false;
    }

    itemPrice = price;
    itemTax = tax;
    return true;
}

bool SettlementReport::parseItemFeeAdjustments(xmlNodePtr itemFeeAdjustments, float &itemFees)
{
    xmlNodePtr cur;
    bool result;
    float fees;

    if (!itemFeeAdjustments)
    {
        fprintf(stderr, "Null pointer. No itemFeeAdjustments node. Cannot parse.\n");
        return false;
    }

    cur = findNodeInChildren(itemFeeAdjustments, (const xmlChar *)"Fee");

    if (!cur)
    {
        fprintf(stderr,"failed to find node [Fee] in itemFeeAdjustments.\n");
        return false;
    }

    result = parseFeeArray(cur, fees);

    if (!result)
    {
        fprintf(stderr, "failed to parse fee array in itemFeeAdjustments\n");
        return false;
    }

    itemFees = fees;
    return true;
}

bool SettlementReport::parseItemPromotionAdjustment(xmlNodePtr itemPromotionAdjustment, float &itemPromotion)
{
    bool result;
    float promotion;
    string amountType;

    if (!itemPromotionAdjustment)
    {
        fprintf(stderr, "Null pointer. No itemPromotionAdjustment node. Cannot parse.\n");
        return false;
    }

    result = findAndParseAmountAndType(itemPromotionAdjustment, promotion, amountType);

    if (!result)
    {
        fprintf(stderr, "failed to find and parse amount and type in itemPromotionAdjustment\n");
        return false;
    }

    itemPromotion = promotion;
    return true;
}

bool SettlementReport::parseFulfillment(xmlNodePtr fulfillment)
{
    xmlNodePtr cur;
    bool itemFlag = false;
    bool result;

    if (!fulfillment)
    {
        fprintf(stderr, "Null pointer. No fulfillment node. Cannot find and parse adjusted item array.\n");
        return false;
    }

    cur = fulfillment->xmlChildrenNode;
    while (cur)
    {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"AdjustedItem")))
        {
            itemFlag = true;
            result = parseAdjustedItem(cur);

            if (!result)
            {
                fprintf(stderr, "failed to parse node [AdjustedItem] in fulfillment.\n");
                return false;
            }
        }

        cur = xmlNextElementSibling(cur);
    }

    if (!itemFlag)
    {
        fprintf(stderr,"failed to find node [AdjustedItem] in fulfillment.\n");
        return false;
    }

    return true; 
}

bool SettlementReport::parseAdjustedItem(xmlNodePtr adjustedItem)
{
    xmlNodePtr cur;
    bool result;
    bool skuFlag = false, priceFlag = false, feeFlag = false, promotionFlag = false;
    string itemSku;
    float itemPrice, itemTax, itemFees, itemPromotion;

    if (!adjustedItem)
    {
        fprintf(stderr, "Null pointer. No ajustedItem node. Cannot parse.\n");
        return false;
    }

    cur = adjustedItem->xmlChildrenNode;
    while (cur)
    {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"SKU")))
        {
            if (skuFlag)
            {
                fprintf(stderr, "found another node [SKU] in adjustedItem. Unexpected\n");
                return false;
            }

            skuFlag = true;
            itemSku = parseString(cur);
        }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"ItemPriceAdjustments")))
        {
            if (priceFlag)
            {
                fprintf(stderr, "found another node [ItemPriceAdjustments] in adjustedItem. Unexpected\n");
                return false;
            }

            priceFlag = true;
            result = parseItemPriceAdjustments(cur, itemPrice, itemTax);

            if (!result)
            {
                fprintf(stderr, "failed to parse item price adjustments in adjustedItem.\n");
                return false;
            }
        }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"ItemFeeAdjustments")))
        {
            if (feeFlag)
            {
                fprintf(stderr, "found another node [ItemFeeAdjustments] in adjustedItem. Unexpected\n");
                return false;
            }

            feeFlag = true;
            result = parseItemFeeAdjustments(cur, itemFees);
            if (!result)
            {
                fprintf(stderr, "failed to parse item fee adjustments in adjustedItem.\n");
                return false;
            }
        }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"PromotionAdjustment")))
        {
            if (promotionFlag)
            {
                fprintf(stderr, "found another node [PromotionAdjustment] in adjustedItem. Unexpected\n");
                return false;
            }

            promotionFlag = true;
            result = parseItemPromotionAdjustment(cur, itemPromotion);
            if (!result)
            {
                fprintf(stderr, "failed to parse item promotion adjustment in adjustedItem.\n");
                return false;
            }
        }
        cur = xmlNextElementSibling(cur);
    }

    if (!skuFlag)
    {
        fprintf(stderr, "failed to find node [SKU] in adjustedItem.\n");
        return false;
    }

    if (!priceFlag)
    {
        fprintf(stderr, "failed to find node [ItemPriceAdjustments] in adjustedItem.\n");
        return false;
    }

    //ItemFeeAdjustments is optional
    if (!feeFlag)
    {
        itemFees = 0.0;
    }

    //ItemPromotionAdjustment is optional
    if (!promotionFlag)
    {
        itemPromotion = 0.0;
    }

    result = addItemFromRefund(itemSku, itemPrice, itemTax, itemFees, itemPromotion);

    if (!result)
    {
        fprintf(stderr, "failed to add refund to map.\n");
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
        return false;
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
    bool result;
    float amount;
    string amountType;

    if (!fee)
    {
        fprintf(stderr,"Null pointer. No Fee node. Cannot parse fee array.\n");
        return false;
    }

    fees = 0.0;
    while (fee)
    {
        result = findAndParseAmountAndType(fee, amount, amountType);

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

bool SettlementReport::findAndParseItemPrice(xmlNodePtr item, float &itemPrice, float &itemTax)
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

    result = findAndParseComponentArray(cur, itemPrice, itemTax);

    if (!result)
    {
        fprintf(stderr, "failed to find and parse component array in node [ItemPrice].\n");
        return false;
    }

    return true;
}

bool SettlementReport::findAndParseComponentArray(xmlNodePtr itemPrice, float &price, float &tax)
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

    result = parseComponentArray(cur, price, tax);
    if (!result)
    {
        fprintf(stderr, "failed to parse component array in itemPrice.\n");
        return false;
    }

    return true;
}

bool SettlementReport::parseComponentArray(xmlNodePtr component, float &itemPrice, float &itemTax)
{
    bool result;
    float amount, price, tax;
    bool taxFlag;

    if (!component)
    {
        fprintf(stderr, "Null pointer. No component node. Cannot parse component array.\n");
        return false;
    }

    price = 0.0;
    tax = 0.0;
    while (component)
    {
        result = parseComponent(component, amount, taxFlag);
        if (!result)
        {
            fprintf(stderr, "failed to parse component in component array.\n");
            return false;
        }

        if (taxFlag)
        {
            tax += amount;
        }
        else
        {
            price += amount;
        }

        component = xmlNextElementSibling(component);
    }

    itemPrice = price;
    itemTax = tax;
    return true;
}

bool SettlementReport::parseComponent(xmlNodePtr component, float &amount, bool &taxFlag)
{
    bool result;
    string amountType;

    if (!component)
    {
        fprintf(stderr, "Null pointer. No component node. Cannot parse component array.\n");
        return false;
    }

    result = findAndParseAmountAndType(component, amount, amountType);

    if (!result)
    {
        fprintf(stderr, "failed to parse amount and type from component.\n");
        return false;
    }

    taxFlag = false;
    if (!amountType.compare("Tax"))
    {
        taxFlag = true;
    }

    return true;
}

bool SettlementReport::findAndParseAmountAndType(xmlNodePtr amountParent, float &amount, string &amountType)
{
    xmlNodePtr cur;
    string type;
    float amt;
    bool typeFlag = false, amtFlag = false;

    if (!amountParent)
    {
        fprintf(stderr, "Null pointer. No parent node for amount. Cannot find and parse amount and type.\n");
        return false;
    }

    // key
    cur = amountParent->xmlChildrenNode;
    while (cur != NULL)
    {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"Type")))
        {
            if (typeFlag)
            {
                fprintf(stderr, "Found another [Type] node. Not expected.\n");
                return false;
            }
            type = parseString(cur);
            typeFlag = true;
        }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"Amount")))
        {
            if (amtFlag)
            {
                fprintf(stderr, "Found another [Amount] node. Not expected.\n");
                return false;
            }
            amt = parseFloat(cur);
            amtFlag = true;
        }
        cur = xmlNextElementSibling(cur);
    }

    if (!amtFlag)
    {
        fprintf(stderr, "node [Amount] not found in amountParent.\n");
        return false;
    }

    if (!typeFlag)
    {
        fprintf(stderr, "node [Type] not found in amountParent.\n");
        return false;
    }

    amountType = type;
    amount = amt;
    return true;
}

bool SettlementReport::findAndParseFeeArray(xmlNodePtr itemFees, float &fees)
{
    xmlNodePtr cur;
    bool result;

    if (!itemFees)
    {
        fprintf(stderr,"Null pointer. No itemFees node. Cannot find and parse fee array.\n");
        return false;
    }

    cur = findNodeInChildren(itemFees, (const xmlChar *)"Fee");

    if (!cur)
    {
        fprintf(stderr,"node [Fee] not found in itemFees.\n");
        return false;
    }

    result = parseFeeArray(cur, fees);

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
    bool result;

    if (!item)
    {
        fprintf(stderr,"Null pointer. No item node. Cannot find and parse item fees.\n");
        return false;
    }

    //ItemFees
    cur =  findNodeInChildren(item, (const xmlChar *)"ItemFees", true);

    if (!cur)
    {
        //ItemFees is optional.
        itemFees = 0.0;
        return true;
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
    string amountType;

    if (!item)
    {
        fprintf(stderr,"Null pointer. No item node. Cannot find and parse promotion.\n");
        return false;
    }

    cur =  findNodeInChildren(item, (const xmlChar *)"Promotion", true);
    if (!cur)
    {
        // Promotion is an optional node
        itemPromotion = 0.0;
        return true;
    }

    result = findAndParseAmountAndType(cur, amount, amountType);

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

    sku = parseString(cur);
    return true;
}
 
bool SettlementReport::findAndParseQuantity(xmlNodePtr item, int &quantity)
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

    quantity = parseInt(cur);
    return true;
}

bool SettlementReport::parseItemArray(xmlNodePtr item)
{
    xmlNodePtr cur;
    bool result;

    string sku;
    int qty;
    float itemPrice, itemTax, itemFees, itemPromotion;

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

        //Quantity
        result = findAndParseQuantity(item, qty);
        if (!result)
        {
            fprintf(stderr, "failed to find and parse quantity in item.\n");
            return false;
        }

        //ItemPrice
        result = findAndParseItemPrice(item, itemPrice, itemTax);
        if (!result)
        {
            fprintf(stderr,"failed to find and parse item price in item.\n");
            return false;
        }

        //ItemFees 
        result = findAndParseItemFees(item, itemFees);

        if (!result)
        {
            fprintf(stderr,"failed to find and parse item fees in item. sku: %s\n", sku.c_str());
            return false;
        }

        //Promotion (optional)
        result = findAndParsePromotion(item, itemPromotion);
        if (!result)
        {
            fprintf(stderr,"failed to find and parse promotion in item.\n");
            return false;
        }
 
        result = addItemFromOrder(sku, qty, itemPrice, itemTax, itemFees, itemPromotion);

        if (!result)
        {
            fprintf(stderr, "failed to add order.\n");
            return false;
        }

        item = xmlNextElementSibling(item);
    }

    return true;
}

void SettlementReport::dumpItemsFromOrders()
{
    OrderSkuInfo orderSkuInfo;
    int totalQty = 0;
    float revenue, profit, totalRevenue = 0.0, totalTax = 0.0, totalProfit = 0.0;
    map<string, OrderSkuInfo>::iterator iter = orderSkuMap.begin();

    if (iter == orderSkuMap.end())
    {
        cout << "No orders" << endl;
        return;
    }
    
    cout << endl;
    cout << "SKU" << '\t' << "Quantity" << '\t' << "Price" << '\t' << "Fees" << '\t' << "Promotion" << '\t' << "Revenue(Price+Fees+Promotion)" << '\t' << "Cost" << '\t' << "Tax" << '\t' << "Profit" << endl;
    for(iter = orderSkuMap.begin(); iter != orderSkuMap.end(); iter++)
    {
        orderSkuInfo = iter->second;
        revenue = orderSkuInfo.price + orderSkuInfo.fee + orderSkuInfo.promotion;
        profit = revenue + orderSkuInfo.cost;
        cout << iter->first << '\t' << orderSkuInfo.qty << '\t' << orderSkuInfo.price << '\t' << orderSkuInfo.fee << '\t' << orderSkuInfo.promotion << '\t' << revenue << '\t' << orderSkuInfo.cost << '\t' << orderSkuInfo.tax << '\t' << profit << endl;

        totalQty += orderSkuInfo.qty;
        totalRevenue += revenue;
        totalProfit += profit;
        totalTax += orderSkuInfo.tax;
    }

    cout << endl;
    cout << "Total SKUs: " << orderSkuMap.size() << endl;
    cout << "Total quantity: " << totalQty << endl;
    cout << "Total Revenue: " << totalRevenue << endl;
    cout << "Total Tax: " << totalTax << endl;
    cout << "Total Profit: " << totalProfit << ' ' << '\t' << (totalProfit/totalRevenue) * 100 << '%' << endl;
    cout << endl;
}

void SettlementReport::dumpItemsFromRefunds()
{
    RefundSkuInfo refundSkuInfo;
    float refund, totalRefund = 0.0, totalTaxRefund = 0.0;
    map<string, RefundSkuInfo>::iterator iter = refundSkuMap.begin();

    if (iter == refundSkuMap.end())
    {
        cout << "No refunds" << endl;
        return;
    }
    
    cout << endl;
    cout << "SKU" << '\t' << "Item Refund" << '\t' << "Fee Refund" << '\t' << "Promotion" << '\t' << "Refund(Item+Fee+Promotion)" << '\t' << "Tax Refund" << endl;
    for(iter = refundSkuMap.begin(); iter != refundSkuMap.end(); iter++)
    {
        refundSkuInfo = iter->second;
        refund = refundSkuInfo.price + refundSkuInfo.fee + refundSkuInfo.promotion;
        cout << iter->first << '\t' << refundSkuInfo.price << '\t' << refundSkuInfo.fee << '\t' << refundSkuInfo.promotion << '\t' << refund << '\t' << refundSkuInfo.tax << endl;

        totalRefund += refund;
        totalTaxRefund += refundSkuInfo.tax;
    }

    cout << endl;
    cout << "Total SKUs: " << refundSkuMap.size() << endl;
    cout << "Total Item Refund: " << totalRefund << endl;
    cout << "Total Tax Refund: " << totalTaxRefund << endl;
    cout << endl;
}

void SettlementReport::dumpSummary(float &summarySales, float &summaryTax, float &summaryProfit)
{
    summary.dump();
    summarySales = summary.getTotalSales();
    summaryTax = summary.getTotalTax();
    summaryProfit = summary.getTotalProfit();
}

SettlementReport::SettlementReport(string docName, string costFile) : XmlDoc(docName),
    cost(costFile),
    orderSkuMap(),
    refundSkuMap(),
    summary()
{
}

SettlementReport::~SettlementReport()
{
}

bool SettlementReport::parse()
{
    bool result;

    result = cost.parseCostReport();
    if (!result)
    {
        fprintf(stderr, "failed to parse cost.\n");
        return false;
    }

    result = parseDoc();
    if (!result)
    {
        fprintf(stderr, "failed to parse doc.\n");
        return false;
    }

    return true;
}

bool SettlementReport::parseDoc()
{
    bool result;
    xmlNodePtr cur;
    string messageType;

    cur = getRootElement();

    if (!cur)
    {
        fprintf(stderr, "failed to get root element. empty document or no/invalid document\n");
        return false;
    }

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

    if ((xmlStrcmp((const xmlChar *)messageType.c_str(), (const xmlChar *)"SettlementReport")))
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

    messageType = parseString(cur);
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

    result = parseSettlementReport(cur);
    if (!result)
    {
        fprintf(stderr, "failed to parse settlement report in message.\n");
        return false;
    }

    return true;
}

bool SettlementReport::parseSettlementReport(xmlNodePtr settlementReport)
{
    bool result;
    xmlNodePtr cur;
    bool orderResult = true, refundResult = true, otherResult = true, settlementResult = true;

    if (!settlementReport)
    {
        fprintf(stderr, "Null pointer. No settlementReport node. Cannot parse settlementReport.\n");
        return false;
    }

    cur = settlementReport->xmlChildrenNode;
    while (cur)
    {
        if (!(xmlStrcmp(cur->name, (const xmlChar *)"Order")))
        {
            orderResult = parseOrder(cur);
            if (!orderResult)
            {
                fprintf(stderr, "failed to parse node [Order] in settlementReport.\n");
                break;
            }
        }
        else if (!(xmlStrcmp(cur->name, (const xmlChar *)"Refund")))
        {
            refundResult = parseRefund(cur);
            if (!refundResult)
            {
                fprintf(stderr, "failed to parse node [Refund] in settlementReport.\n");
                return false;
            }
        }
        else if (!(xmlStrcmp(cur->name, (const xmlChar *)"OtherTransaction")))
        {
            otherResult = parseOtherTransaction(cur);
            if (!otherResult)
            {
                fprintf(stderr, "failed to parse node [OtherTransaction] in settlementReport.\n");
                return false;
            }
        }
        else if (!(xmlStrcmp(cur->name, (const xmlChar *)"SettlementData")))
        {
            settlementResult = parseSettlementData(cur);
            if (!settlementResult)
            {
                fprintf(stderr, "failed to parse node [OtherTransaction] in settlementReport.\n");
                return false;
            }
        }

        cur = xmlNextElementSibling(cur);
    }

    if (!orderResult)
    {
        fprintf(stderr, "failed to parse order in settlementReport.\n");
        return false;
    }

    if (!refundResult)
    {
        fprintf(stderr, "failed to parse order in settlementReport.\n");
        return false;
    }

    if (!otherResult)
    {
        fprintf(stderr, "failed to parse other transactions in settlementReport.\n");
        return false;
    }

    if (!settlementResult)
    {
        fprintf(stderr, "failed to parse settlement data in settlementReport.\n");
        return false;
    }
 
    return true;
}

int main(int argc, char **argv)
{
    bool result;
    string file, ext = ".txt", xmlDir = "xmlData", year="2017";
    string settlementFile, costFile;
    struct dirent *entry;
    DIR *dir;
    size_t extLen, fileLen;
    float ytdSales = 0.0, ytdTax = 0.0, ytdProfit = 0.0;
    float summarySales, summaryTax, summaryProfit;
    
    int count = 0;

    if (argc > 1)
    {
        year = argv[1];
        if (year.compare("2015") || year.compare("2016") || year.compare("2017"))
        {
            fprintf(stderr, "invalid year [%s]. Enter 2015, 2016 or 2017\n", xmlDir.c_str());
            return 1;
        }
    }

    dir = opendir((xmlDir + '/' + year).c_str());
    if (dir == NULL)
    {
        fprintf(stderr, "failed to open directory [%s]\n", xmlDir.c_str());
        return 3;
    }

    costFile = xmlDir;
    costFile.append("/cost.xml");

    printf("Year: %s\n", year.c_str());
    extLen = ext.length();
    while ((entry = readdir(dir)) != NULL)
    {
        file = entry->d_name;
        cout << "filename: " << file << endl;

        if (!file.compare(".") || !file.compare(".."))
        {
            continue;
        }

        fileLen = file.length();

        if (fileLen <= extLen)
        {
            fprintf(stderr, "invalid file [%s]. file name is shorter than extension [%s]. %zu <= %zu\n", file.c_str(), ext.c_str(), fileLen, extLen);
            closedir(dir);
            return 5;
        }

        if (file.compare(fileLen - extLen, string::npos, ext))
        {
            fprintf(stderr, "invalid extension in file: [%s]. expected extension [%s].\n", file.c_str(), ext.c_str());
            closedir(dir);
            return 6;
        }

        settlementFile = xmlDir;
        settlementFile.push_back('/');
        settlementFile.append(year);
        settlementFile.push_back('/');
#if 1
        // 2017
        //file = "5395199049017327.txt";
        //file = "5251691949017313.txt";
        //file = "5111340848017299.txt";
        //file = "4967589632017285.txt";
        //file = "4827028021017271.txt";
        //file = "4690852492017257.txt";
        //file = "4552318323017243.txt";
        //file = "5446451252017332.txt";
        //file = "5447669528017332.txt";
        //file = "5451550576017332.txt";
        //file = "5448586679017332.txt";
        //file = "5449381762017332.txt";

        // 2016
        //file = "5443749205017332.txt";
        //file = "5444703250017332.txt";
        //file = "5444803643017332.txt";
        //file = "5445640987017332.txt";
        //file = "5445663375017332.txt";
        //file = "5445853021017332.txt";
        //file = "5446245398017332.txt";
        //file = "5446538233017332.txt";
        //file = "5446706517017332.txt";
        //file = "5447030431017332.txt";
        //file = "5447114658017332.txt";
        //file = "5447132255017332.txt";
        //file = "5447248333017332.txt";
        //file = "5447254468017332.txt";
        //file = "5447313015017332.txt";
        //file = "5447378612017332.txt";
        //file = "5448182219017332.txt";
        //file = "5448284411017332.txt";
        //file = "5448491286017332.txt";
        //file = "5448584445017332.txt";
        //file = "5448729676017332.txt";
        //file = "5448893214017332.txt";
        //file = "5449116203017332.txt";
        //file = "5449587330017332.txt";
        //file = "5450274724017332.txt";
        //file = "5450883419017332.txt";
        //file = "5452808768017332.txt";

        // 2015
		//file = "5443798738017332.txt";
		//file = "5444980647017332.txt";
		//file = "5445050108017332.txt";
		//file = "5445813868017332.txt";
		//file = "5445841061017332.txt";
		//file = "5446553936017332.txt";
		//file = "5447472371017332.txt";
		//file = "5447585427017332.txt";
		//file = "5447630798017332.txt";
		//file = "5447785759017332.txt";
		//file = "5448163727017332.txt";
		//file = "5448303716017332.txt";
		//file = "5448343359017332.txt";
		//file = "5449306401017332.txt";
		//file = "5450111231017332.txt";
		//file = "5451848932017332.txt";
#endif

        settlementFile.append(file);
        cout << "File: " << settlementFile << endl;
        SettlementReport report(settlementFile, costFile);
        printf("\n");
#if 0
        if (argc <= 1) {
            printf("Usage: %s docname\n", argv[0]);
            closedir(dir);
            return(0);
        }
        docname = argv[1];
#endif

        result = report.parse();
        if (!result)
        {
            fprintf(stderr, "failed to parse Amazon envelope.\n");
            closedir(dir);
            return 2;
        }
#ifdef DEBUG
        report.dumpItemsFromOrders();
        report.dumpItemsFromRefunds();
#endif
        report.dumpSummary(summarySales, summaryTax, summaryProfit);
        ytdSales += summarySales;
        ytdTax += summaryTax;
        ytdProfit += summaryProfit;
        count++;
    }

    if (!count)
    {
        fprintf(stderr, "no files in directory %s\n", xmlDir.c_str());
        closedir(dir);
        return 4;
    }
 
    closedir(dir);

    printf("Parsed %d files.\n", count);
    cout << endl;
    cout << "YTD Sales [" << year << "]:  " << ytdSales << endl;
    cout << "YTD Tax Collected [" << year << "]:  " << ytdTax << endl;
    cout << "YTD Profit [" << year << "]:  " << ytdProfit << endl;
    printf("\n");
    return 0;
}
