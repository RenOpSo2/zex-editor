# Compiler & Flags
CC        = gcc
CXX       = g++
CFLAGS ?= -Wall -Wextra -Wpedantic -O2 -std=gnu99 -I.
CXXFLAGS ?= -Wall -Wextra -Wpedantic -O2 -std=c++17 -I.
.DEFAULT_GOAL := all

# Directories
SRCDIR    = src
BUILDDIR  = build
BINDIR    = bin

# Target
TARGET    = $(BINDIR)/zex

# Sources
C_SRCS    = $(filter-out $(SRCDIR)/cmd.c, $(wildcard $(SRCDIR)/*.c)) libmemory/arena.c
CPP_SRCS  = $(wildcard $(SRCDIR)/*.cpp)
C_OBJS    = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(filter-out $(SRCDIR)/cmd.c, $(wildcard $(SRCDIR)/*.c))) \
            $(BUILDDIR)/libmemory/arena.o
CPP_OBJS  = $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(CPP_SRCS))
OBJS      = $(C_OBJS) $(CPP_OBJS)
LIB_OBJS  = $(filter-out $(BUILDDIR)/main.o, $(OBJS))

# Test targets
TEST_UNDO_TARGET       = $(BINDIR)/test_undo
TEST_ENHANCED_UNDO_TARGET = $(BINDIR)/test_enhanced_undo
TEST_LARGE_UNDO_TARGET = $(BINDIR)/test_large_undo
TEST_020_TARGET        = $(BINDIR)/test_0.2.0
TEST_CONFIG_TARGET     = $(BINDIR)/test_config
TEST_STRESS_TARGET     = $(BINDIR)/test_stress
TEST_EDGE_TARGET       = $(BINDIR)/test_edge
TEST_CURSOR_TARGET     = $(BINDIR)/test_cursor
TEST_CPP_BRIDGE_TARGET = $(BINDIR)/test_cpp_bridge
TEST_FILE_IO_TARGET    = $(BINDIR)/test_file_io
TEST_SEARCH_TARGET     = $(BINDIR)/test_search

TEST_C_SRCS = tests/test_undo.c tests/test_enhanced_undo.c tests/test_large_undo.c tests/test_0.2.0.c tests/test_config.c tests/stress_test.c tests/edge_case_test.c tests/test_cursor.c tests/test_file_io.c tests/test_search.c
TEST_CPP_SRCS = tests/test_cpp_bridge.cpp
TEST_C_OBJS = $(patsubst tests/%.c, $(BUILDDIR)/tests/%.o, $(TEST_C_SRCS))
TEST_CPP_OBJS = $(patsubst tests/%.cpp, $(BUILDDIR)/tests/%.o, $(TEST_CPP_SRCS))
DEPS = $(OBJS:.o=.d) $(TEST_C_OBJS:.o=.d) $(TEST_CPP_OBJS:.o=.d)

# Phony targets
.PHONY: all clean run format format-astyle dirs test test-stress test-edge test-cursor test-cpp-bridge test-file-io test-search bench-search check help

bench-search: dirs
	@$(CC) $(CFLAGS) bench_search.c -o $(BINDIR)/bench_search
	@$(BINDIR)/bench_search

# Default target
all: dirs $(TARGET)

# Create directories
dirs:
	@mkdir -p $(BUILDDIR) $(BINDIR)

# Link executable
$(TARGET): $(OBJS)
	@echo "Linking $@..."
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

# Link tests
$(TEST_UNDO_TARGET): $(BUILDDIR)/tests/test_undo.o $(LIB_OBJS)
	@echo "Linking undo test..."
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

$(TEST_ENHANCED_UNDO_TARGET): $(BUILDDIR)/tests/test_enhanced_undo.o $(LIB_OBJS)
	@echo "Linking enhanced undo test..."
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

$(TEST_LARGE_UNDO_TARGET): $(BUILDDIR)/tests/test_large_undo.o $(LIB_OBJS)
	@echo "Linking large undo test..."
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

$(TEST_020_TARGET): $(BUILDDIR)/tests/test_0.2.0.o $(LIB_OBJS)
	@echo "Linking 0.2.0 test..."
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

$(TEST_CONFIG_TARGET): $(BUILDDIR)/tests/test_config.o $(LIB_OBJS)
	@echo "Linking config test..."
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

$(TEST_STRESS_TARGET): $(BUILDDIR)/tests/stress_test.o $(LIB_OBJS)
	@echo "Linking stress test..."
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

$(TEST_EDGE_TARGET): $(BUILDDIR)/tests/edge_case_test.o $(LIB_OBJS)
	@echo "Linking edge case test..."
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

$(TEST_CURSOR_TARGET): $(BUILDDIR)/tests/test_cursor.o $(LIB_OBJS)
	@echo "Linking cursor test..."
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

$(TEST_CPP_BRIDGE_TARGET): $(BUILDDIR)/tests/test_cpp_bridge.o $(LIB_OBJS)
	@echo "Linking C++ bridge test..."
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

$(TEST_FILE_IO_TARGET): $(BUILDDIR)/tests/test_file_io.o $(LIB_OBJS)
	@echo "Linking file I/O test..."
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

$(TEST_SEARCH_TARGET): $(BUILDDIR)/tests/test_search.o $(LIB_OBJS)
	@echo "Linking search test..."
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

# Compile objects with dependency tracking
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/libmemory/%.o: libmemory/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/tests/%.o: tests/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Include auto-generated dependencies
-include $(DEPS)

# Run the program
run: all
	@echo "Running $(TARGET)..."
	@$(TARGET)

# Clean build artifacts
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILDDIR) $(BINDIR)
	@echo "Clean complete!"

# Format source code
format:
	@echo "Formatting source files..."
	@clang-format -i $(SRCDIR)/*.c $(SRCDIR)/*.cpp $(SRCDIR)/*.h tests/*.c tests/*.cpp
	@echo "Format complete!"

# Format source code with astyle (K&R style, 4-space indent)
format-astyle:
	@echo "Formatting source files with astyle (K&R, 4-space indent)..."
	@astyle --style=kr --indent=spaces=4 --convert-tabs --pad-oper \
	         --pad-header --unpad-paren --align-pointer=type \
	         $(SRCDIR)/*.c $(SRCDIR)/*.cpp $(SRCDIR)/*.h \
	         libmemory/*.c libmemory/*.h tests/*.c tests/*.cpp
	@echo "Astyle format complete!"

# Check formatting without changing files
format-check:
	@echo "Checking format..."
	@clang-format --dry-run --Werror $(SRCDIR)/*.c $(SRCDIR)/*.cpp $(SRCDIR)/*.h tests/*.c tests/*.cpp

# Static analysis with cppcheck (if installed)
check:
	@echo "Running cppcheck..."
	@cppcheck --enable=all --suppress=missingIncludeSystem $(SRCDIR)/

# Show help
help:
	@echo "Available targets:"
	@echo "  all              : Build the project (default)"
	@echo "  run              : Build and run"
	@echo "  test             : Build and run all tests"
	@echo "  test-stress      : Build and run stress test only"
	@echo "  test-edge        : Build and run edge case test only"
	@echo "  test-cursor      : Build and run cursor test only"
	@echo "  test-cpp-bridge  : Build and run the C++ bridge smoke test"
	@echo "  test-enhanced-undo: Build and run enhanced undo test only"
	@echo "  test-large-undo  : Build and run large undo test only"
	@echo "  test-file-io     : Build and run file I/O test only"
	@echo "  test-search      : Build and run search test only"
	@echo "  clean            : Remove build artifacts"
	@echo "  format           : Format source with clang-format"
	@echo "  format-astyle    : Format source with astyle (K&R, 4-space indent)"
	@echo "  format-check     : Check formatting without changes"
	@echo "  check            : Static analysis with cppcheck"
	@echo "  help             : Show this help"

# Individual test targets
test-stress: dirs $(TEST_STRESS_TARGET)
	@echo "Running stress test..."
	@$(TEST_STRESS_TARGET)

test-edge: dirs $(TEST_EDGE_TARGET)
	@echo "Running edge case test..."
	@$(TEST_EDGE_TARGET)

test-cursor: dirs $(TEST_CURSOR_TARGET)
	@echo "Running cursor accuracy test..."
	@$(TEST_CURSOR_TARGET)

test-cpp-bridge: dirs $(TEST_CPP_BRIDGE_TARGET)
	@echo "Running C++ bridge smoke test..."
	@$(TEST_CPP_BRIDGE_TARGET)

test-enhanced-undo: dirs $(TEST_ENHANCED_UNDO_TARGET)
	@echo "Running enhanced undo test..."
	@$(TEST_ENHANCED_UNDO_TARGET)

test-large-undo: dirs $(TEST_LARGE_UNDO_TARGET)
	@echo "Running large undo test..."
	@$(TEST_LARGE_UNDO_TARGET)

test-file-io: dirs $(TEST_FILE_IO_TARGET)
	@echo "Running file I/O test..."
	@$(TEST_FILE_IO_TARGET)

test-search: dirs $(TEST_SEARCH_TARGET)
	@echo "Running search test..."
	@$(TEST_SEARCH_TARGET)

# Run tests
test: dirs $(TEST_UNDO_TARGET) $(TEST_ENHANCED_UNDO_TARGET) $(TEST_LARGE_UNDO_TARGET) $(TEST_020_TARGET) $(TEST_CONFIG_TARGET) $(TEST_STRESS_TARGET) $(TEST_EDGE_TARGET) $(TEST_CURSOR_TARGET) $(TEST_CPP_BRIDGE_TARGET) $(TEST_FILE_IO_TARGET) $(TEST_SEARCH_TARGET)
	@echo "Running test_undo..."
	@$(TEST_UNDO_TARGET)
	@echo "Running test_enhanced_undo..."
	@$(TEST_ENHANCED_UNDO_TARGET)
	@echo "Running test_large_undo..."
	@$(TEST_LARGE_UNDO_TARGET)
	@echo "Running test_0.2.0..."
	@$(TEST_020_TARGET)
	@echo "Running test_config..."
	@$(TEST_CONFIG_TARGET)
	@echo "Running test_stress..."
	@$(TEST_STRESS_TARGET)
	@echo "Running test_edge..."
	@$(TEST_EDGE_TARGET)
	@echo "Running test_cursor..."
	@$(TEST_CURSOR_TARGET)
	@echo "Running test_cpp_bridge..."
	@$(TEST_CPP_BRIDGE_TARGET)
	@echo "Running test_file_io..."
	@$(TEST_FILE_IO_TARGET)
	@echo "Running test_search..."
	@$(TEST_SEARCH_TARGET)