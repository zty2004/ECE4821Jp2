CXX = g++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra -Wconversion -Wno-unused-result -Wvla -Wpedantic
TARGET = bin/lemondb-2025
SOURCES = $(shell find src -name "*.cpp")

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

.PHONY: all clean
