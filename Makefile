# Compiler and flags
CC = gcc
CFLAGS = -std=c11 -pthread -Wall -Iinclude -g

# File structure
SRC_DIR = src
OBJ_DIR = build
BIN_DIR = bin
TEST_DIR = test
TARGET = $(BIN_DIR)/diningPhilosophers
TEST_TARGET = $(BIN_DIR)/testRunner
TEST_SRCS = test/TestDining.c src/DiningPhilosophers.c

# Source and Object files
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/DiningPhilosophers.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Default rule
.PHONY: all
all: $(TARGET)

# Create the binary in BIN_DIR
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Compile each src .c into build/*.o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile each test .c into build
$(OBJ_DIR)/%.o: $(TEST_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


# Ensure build and bin directories exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Test rule
.PHONY: test
test: | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRCS) -lcmocka -pthread
	./$(TEST_TARGET)

# Obviously, clean rule
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
