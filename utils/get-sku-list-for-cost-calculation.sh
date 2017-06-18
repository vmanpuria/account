sort -u /tmp/del > /tmp/del.sort
sed -e 's/sku \[//g' -i .bak /tmp/del.sort
sed -e 's/] not found in cost map.//g' -i bak /tmp/del.sort
cat /tmp/del.sort >> xmlData/cost.xml
