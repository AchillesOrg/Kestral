CXX ?= c++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic
TARGET := build/kestral
TEST_TARGET := build/test_kestral

LIB_SRCS := src/lexer.cpp src/parser.cpp src/interp.cpp src/builtins.cpp src/nana.cpp src/learn.cpp src/repl.cpp
MAIN_SRC := src/main.cpp
TEST_SRC := tests/test_kestral.cpp

.PHONY: all test install clean

all: $(TARGET)

$(TARGET): $(LIB_SRCS) $(MAIN_SRC)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc $^ -o $@

$(TEST_TARGET): $(LIB_SRCS) $(TEST_SRC)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc $^ -o $@

test: $(TEST_TARGET) $(TARGET)
	./$(TEST_TARGET)

install: $(TARGET)
	@mkdir -p $(HOME)/.local/bin
	cp $(TARGET) $(HOME)/.local/bin/kestral
	chmod +x $(HOME)/.local/bin/kestral
	@echo "Installed kestral to $(HOME)/.local/bin/kestral"

clean:
	rm -rf build
