# Compiler and flags
CC = gcc
CFLAGS = -std=c11 -Wall -Iinclude -g -MMD -MP -D_POSIX_C_SOURCE=200809L
LDFLAGS = -pthread
COVERAGE_FLAGS = -O0 --coverage

LDFLAGS_TEST = -lcmocka $(LDFLAGS)
LDFLAGS_COVERAGE = --coverage $(LDFLAGS_TEST)

# File structure
SRC_DIR = src
OBJ_DIR = build
BIN_DIR = bin
TEST_DIR = test
MOCK_DIR = $(TEST_DIR)/mocks
COVERAGE_DIR = cvg

# Targets
TARGET = $(BIN_DIR)/diningPhilosophers
TEST_TARGET = $(BIN_DIR)/testRunner

# Sources
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/DiningPhilosophers.c
TEST_SRCS = $(TEST_DIR)/TestDining.c $(SRC_DIR)/DiningPhilosophers.c
MOCK_SRCS = $(wildcard $(MOCK_DIR)/*.c)

# Objects
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:%.c=$(OBJ_DIR)/%.o)
MOCK_OBJS = $(MOCK_SRCS:%.c=$(OBJ_DIR)/%.o)

# Look for main.c in src/
vpath %.c $(SRC_DIR)

# Collect all object files for dependency inclusion
ALL_OBJS = $(OBJS) $(TEST_OBJS) $(MOCK_OBJS)

# Include all auto-generated dependencies
-include $(ALL_OBJS:.o=.d)

.PHONY: all clean test test_mock coverage

all: $(TARGET)
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile Rules
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure needed directories exist
$(OBJ_DIR) $(BIN_DIR) $(COVERAGE_DIR):
	mkdir -p $@

# Tests
test: $(TEST_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(TEST_TARGET) $(LDFLAGS_TEST)

# Tests with mocks
# test_mock: | $(BIN_DIR)
# 	$(CC) $(CFLAGS) $(TEST_SRCS) $(MOCK_SRCS) -o $(TEST_TARGET) $(LDFLAGS_TEST)

# Run coverage
coverage: clean | $(BIN_DIR) $(COVERAGE_DIR)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) $(TEST_SRCS) -o $(TEST_TARGET) $(LDFLAGS_COVERAGE)
	./$(TEST_TARGET)
	gcovr --root . --html-details -o $(COVERAGE_DIR)/coverage_report.html

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(COVERAGE_DIR)
