# Compiler & Flags
CC        = gcc
CFLAGS = -Wall -Wextra -Wpedantic -g -O0 -std=gnu99

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
DEPS      = $(OBJS:.o=.d) $(BUILDDIR)/test_undo.d

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

# Phony targets
.PHONY: all clean run format format-astyle dirs test

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
	@echo "  test         : Build and run undo/redo test"
	@echo "  clean        : Remove build artifacts"
	@echo "  format       : Format source with clang-format"
	@echo "  format-astyle: Format source with astyle (K&R, 4-space indent)"
	@echo "  format-check : Check formatting without changes"
	@echo "  check        : Static analysis with cppcheck"
	@echo "  help         : Show this help"

# Run tests
test: dirs $(TEST_UNDO_TARGET) $(TEST_020_TARGET) $(TEST_CONFIG_TARGET)
	@echo "Running test_undo..."
	@$(BINDIR)/test_undo
	@echo "Running test_0.2.0..."
	@$(BINDIR)/test_0.2.0
	@echo "Running test_config..."
	@$(BINDIR)/test_config
