CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
TARGET = vdiskcat
SOURCES = vdiskcat.cpp
HEADERS = vdiskcat.h

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

.PHONY: clean install