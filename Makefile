# Executable name
TARGET = Bloody-Path

# Compiler
CXX = g++

# Source files
SRCS = main.cpp map_parser.cpp renderer.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

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

# Compiling rule: create object files from .cpp source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule: remove generated files
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
