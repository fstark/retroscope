CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -O2
# CXXFLAGS = -std=c++23 -Wall -Wextra -O0 -g
TARGET = vdiskcat
SOURCES = vdiskcat.cpp
HEADERS = vdiskcat.h hfs.h

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

.PHONY: clean install