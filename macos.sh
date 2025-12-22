#!/bin/bash

# macOS build script for Bloody Path
echo "Building Bloody Path for macOS..."

# Check for Homebrew
if ! command -v brew &> /dev/null
then
    echo "❌ Homebrew not found. Please install Homebrew first:"
    echo '/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
    exit 1
fi

# Check for SDL2
if ! brew list sdl2 &> /dev/null
then
    echo "📦 SDL2 not found. Installing SDL2..."
    brew install sdl2
else
    echo "✅ SDL2 is installed"
fi

# Check for GLEW
if ! brew list glew &> /dev/null
then
    echo "📦 GLEW not found. Installing GLEW..."
    brew install glew
else
    echo "✅ GLEW is installed"
fi

# Build using Makefile
echo "🔨 Building with make..."
make clean
make

# Check if compilation was successful
if [ $? -eq 0 ]; then
else
    echo "Compilation failed."
    exit 1
fi
