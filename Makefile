# Compiler & Flags
CC        = gcc
CFLAGS ?= -Wall -Wextra -Wpedantic -O2 -std=gnu99
.DEFAULT_GOAL := all

# Directories
SRCDIR    = src
BUILDDIR  = build
BINDIR    = bin

# Target
TARGET    = $(BINDIR)/zex

# Source files (exclude cmd.c — removed in favour of Ctrl+S/Q)
SRCS      = $(filter-out $(SRCDIR)/cmd.c, $(wildcard $(SRCDIR)/*.c)) libmemory/arena.c
OBJS      = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(filter-out $(SRCDIR)/cmd.c, $(wildcard $(SRCDIR)/*.c))) \
            $(BUILDDIR)/libmemory/arena.o
DEPS      = $(OBJS:.o=.d) $(BUILDDIR)/test_undo.d $(BUILDDIR)/test_0.2.0.d $(BUILDDIR)/test_config.d $(BUILDDIR)/tests/stress_test.d $(BUILDDIR)/tests/edge_case_test.d

# Test targets
TEST_UNDO_SRCS = test_undo.c
TEST_UNDO_OBJS = $(BUILDDIR)/test_undo.o $(filter-out $(BUILDDIR)/main.o, $(OBJS))
TEST_UNDO_TARGET = $(BINDIR)/test_undo

TEST_020_SRCS = test_0.2.0.c
TEST_020_OBJS = $(BUILDDIR)/test_0.2.0.o $(filter-out $(BUILDDIR)/main.o, $(OBJS))
TEST_020_TARGET = $(BINDIR)/test_0.2.0

TEST_CONFIG_SRCS = test_config.c
TEST_CONFIG_OBJS = $(BUILDDIR)/test_config.o $(filter-out $(BUILDDIR)/main.o, $(OBJS))
TEST_CONFIG_TARGET = $(BINDIR)/test_config

TEST_STRESS_SRCS = tests/stress_test.c
TEST_STRESS_OBJS = $(BUILDDIR)/tests/stress_test.o $(filter-out $(BUILDDIR)/main.o, $(OBJS))
TEST_STRESS_TARGET = $(BINDIR)/test_stress

TEST_EDGE_SRCS = tests/edge_case_test.c
TEST_EDGE_OBJS = $(BUILDDIR)/tests/edge_case_test.o $(filter-out $(BUILDDIR)/main.o, $(OBJS))
TEST_EDGE_TARGET = $(BINDIR)/test_edge

# Phony targets
.PHONY: all clean run format format-astyle dirs test test-stress test-edge bench-search

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
	@$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

# Build undo test
$(TEST_UNDO_TARGET): $(TEST_UNDO_OBJS)
	@echo "Linking undo test..."
	@$(CC) $(CFLAGS) $(TEST_UNDO_OBJS) -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

# Build 0.2.0 test
$(TEST_020_TARGET): $(TEST_020_OBJS)
	@echo "Linking 0.2.0 test..."
	@$(CC) $(CFLAGS) $(TEST_020_OBJS) -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

# Build config test
$(TEST_CONFIG_TARGET): $(TEST_CONFIG_OBJS)
	@echo "Linking config test..."
	@$(CC) $(CFLAGS) $(TEST_CONFIG_OBJS) -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

# Build stress test
$(TEST_STRESS_TARGET): $(TEST_STRESS_OBJS)
	@echo "Linking stress test..."
	@$(CC) $(CFLAGS) $(TEST_STRESS_OBJS) -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

# Build edge case test
$(TEST_EDGE_TARGET): $(TEST_EDGE_OBJS)
	@echo "Linking edge case test..."
	@$(CC) $(CFLAGS) $(TEST_EDGE_OBJS) -o $@ $(LDFLAGS)
	@echo "Test build complete: $@"

# Compile test objects
$(BUILDDIR)/test_undo.o: test_undo.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/test_0.2.0.o: test_0.2.0.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/test_config.o: test_config.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/tests/stress_test.o: tests/stress_test.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/tests/edge_case_test.o: tests/edge_case_test.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Compile objects with dependency tracking
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/libmemory/%.o: libmemory/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

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

# Format source code with clang-format
format:
	@echo "Formatting source files..."
	@clang-format -i $(SRCDIR)/*.c $(SRCDIR)/*.h
	@echo "Format complete!"

# Format source code with astyle (K&R style, 4-space indent)
format-astyle:
	@echo "Formatting source files with astyle (K&R, 4-space indent)..."
	@astyle --style=kr --indent=spaces=4 --convert-tabs --pad-oper \
	         --pad-header --unpad-paren --align-pointer=type \
	         $(SRCDIR)/*.c $(SRCDIR)/*.h libmemory/*.c libmemory/*.h
	@echo "Astyle format complete!"

# Check formatting without changing files
format-check:
	@echo "Checking format..."
	@clang-format --dry-run --Werror $(SRCDIR)/*.c $(SRCDIR)/*.h

# Static analysis with cppcheck (if installed)
check:
	@echo "Running cppcheck..."
	@cppcheck --enable=all --suppress=missingIncludeSystem $(SRCDIR)/

# Show help
help:
	@echo "Available targets:"
	@echo "  all          : Build the project (default)"
	@echo "  run          : Build and run"
	@echo "  test         : Build and run all tests"
	@echo "  test-stress  : Build and run stress test only"
	@echo "  test-edge    : Build and run edge case test only"
	@echo "  clean        : Remove build artifacts"
	@echo "  format       : Format source with clang-format"
	@echo "  format-astyle: Format source with astyle (K&R, 4-space indent)"
	@echo "  format-check : Check formatting without changes"
	@echo "  check        : Static analysis with cppcheck"
	@echo "  help         : Show this help"

# Individual test targets
test-stress: dirs $(TEST_STRESS_TARGET)
	@echo "Running stress test..."
	@$(BINDIR)/test_stress

test-edge: dirs $(TEST_EDGE_TARGET)
	@echo "Running edge case test..."
	@$(BINDIR)/test_edge

# Run tests
test: dirs $(TEST_UNDO_TARGET) $(TEST_020_TARGET) $(TEST_CONFIG_TARGET) $(TEST_STRESS_TARGET) $(TEST_EDGE_TARGET)
	@echo "Running test_undo..."
	@$(BINDIR)/test_undo
	@echo "Running test_0.2.0..."
	@$(BINDIR)/test_0.2.0
	@echo "Running test_config..."
	@$(BINDIR)/test_config
	@echo "Running test_stress..."
	@$(BINDIR)/test_stress
	@echo "Running test_edge..."
	@$(BINDIR)/test_edge
