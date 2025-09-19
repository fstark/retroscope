CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
TARGET = vdiskcat
SOURCE = vdiskcat.cpp

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

.PHONY: clean install