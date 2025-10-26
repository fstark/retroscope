CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -O2 -g -I.
# CXXFLAGS = -std=c++23 -Wall -Wextra -O0 -g -I.
DEPFLAGS = -MMD -MP
MAKEFLAGS += -j12

TARGET = retroscope
SOURCES = retroscope.cpp utils.cpp file/file.cpp file/folder.cpp file/disk.cpp file/file_visitor.cpp file/file_set.cpp partition.cpp hfs/hfs_partition.cpp hfs/hfs_fork.cpp mfs/mfs_partition.cpp data/apm_datasource.cpp data/dc42_datasource.cpp data/stripped_datasource.cpp data/bin_datasource.cpp rsrc/rsrc.cpp rsrc/rsrc_parser.cpp
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