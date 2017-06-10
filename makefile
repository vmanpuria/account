all:
	$(CXX) -o parse settlement_report.cpp -lxml2 -I/usr/local/Cellar/libxml2/2.9.4_3/include/libxml2
#	$(CC) -o parse settlement_report.cpp -lxml2 -I/usr/local/Cellar/libxml2/2.9.4_3/include/libxml2
