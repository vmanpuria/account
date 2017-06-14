all:
	$(CXX) -o parse xml_doc.cpp cost.cpp settlement_report.cpp -lxml2 -I/usr/local/Cellar/libxml2/2.9.4_3/include/libxml2 -DDEBUG
