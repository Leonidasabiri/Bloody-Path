#!/bin/bash

# Check for Homebrew
if ! command -v brew &> /dev/null
then
    echo "Homebrew not found. Please install Homebrew to continue by running:"
    echo '/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
    exit 1
fi

# Check for SDL2
if ! brew list sdl2 &> /dev/null
then
    echo "SDL2 not found. Installing SDL2 with Homebrew..."
    brew install sdl2
fi

# Check for sdl2-config
if ! command -v sdl2-config &> /dev/null
then
    echo "sdl2-config not found. Make sure SDL2 is installed correctly and sdl2-config is in your PATH."
    exit 1
fi

# Get compiler and linker flags from sdl2-config
CFLAGS=$(sdl2-config --cflags)
LIBS=$(sdl2-config --libs)

# Compile the application
echo "Compiling main.cpp with flags:"
echo "CFLAGS: $CFLAGS"
echo "LIBS: $LIBS"
g++ -std=c++11 main.cpp map_parser.cpp renderer.cpp -o Bloody-Path $CFLAGS $LIBS

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful. Running the application..."
    ./Bloody-Path
else
    echo "Compilation failed."
    exit 1
fi
