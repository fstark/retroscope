CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -O2
# CXXFLAGS = -std=c++23 -Wall -Wextra -O0 -g
DEPFLAGS = -MMD -MP

TARGET = retroscope
SOURCES = retroscope.cpp utils.cpp file.cpp hfs_parser.cpp apm_datasource.cpp dc42_datasource.cpp stripped_data_source.cpp bin_datasource.cpp
OBJECTS = $(SOURCES:.cpp=.o)
DEPS = $(SOURCES:.cpp=.d)

# Default target
all: $(TARGET)

# Include dependency files
-include $(DEPS)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Compile each source file to object file and generate dependencies
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPS)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

.PHONY: all clean install