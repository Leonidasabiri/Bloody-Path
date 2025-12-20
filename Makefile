# Executable name
TARGET = Bloody-Path
TILESET_TEST = tileset_test

# Compiler
CXX = g++

# Source files
SRCS = main.cpp map_parser.cpp renderer.cpp
TEST_SRCS = tileset_test.cpp map_parser.cpp renderer.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

# Compiler and linker flags from sdl2-config
# -std=c++11 is required for the lambda function in main.cpp
# -Wall -Wextra -g are good practice for warnings and debugging
CXXFLAGS = -std=c++11 -Wall -Wextra -g $(shell sdl2-config --cflags)
LIBS = $(shell sdl2-config --libs) -lSDL2_image

# Default rule: build everything
.PHONY: all
all: $(TARGET)

# Linking rule: create the executable from object files
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LIBS)

# Tileset test target
.PHONY: test
test: $(TILESET_TEST)

$(TILESET_TEST): tileset_test.o map_parser.o renderer.o
	$(CXX) tileset_test.o map_parser.o renderer.o -o $(TILESET_TEST) $(LIBS)

# Compiling rule: create object files from .cpp source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule: remove generated files
.PHONY: clean
clean:
	rm -f $(OBJS) tileset_test.o $(TARGET) $(TILESET_TEST)

