#!/bin/bash
# runvalgrind.sh - Run valgrind on a specified binary (default: bin/testRunner)

# Use the first argument as the binary, or default
TEST_BIN=${1:-bin/testRunner}

# Shift positional arguments so $@ contains anything after the binary
shift

# Check if the binary exists
if [[ ! -f $TEST_BIN ]]; then
    echo "Binary not found: $TEST_BIN"
    echo "Usage: $0 [path_to_binary] [binary_args]"
    exit 1
fi

# Run valgrind
valgrind \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    --verbose \
    "$TEST_BIN" "$@"