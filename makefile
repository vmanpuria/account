SRC = xml_doc.cpp cost.cpp settlement_report.cpp
CXXFLAGS = -I/usr/local/opt/libxml2/include/libxml2 -I/usr/local/opt/boost/include
LIBS = -lxml2

all:
	$(CXX) -o parse $(SRC) $(LIBS) $(CXXFLAGS)
