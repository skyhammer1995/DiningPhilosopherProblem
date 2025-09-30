#!/bin/bash
# runvalgrind.sh - Run valgrind on a specified binary (default: bin/testRunner)

# Default values
TOOL="memcheck"
TEST_BIN="bin/testRunner"

# Parse tool flag
if [[ $1 == "--helgrind" ]]; then
    TOOL="helgrind"
    shift
elif [[ $1 == "--drd" ]]; then
    TOOL="drd"
    shift
fi

# check if the binary path argument is not empty, and if it isn't, make sure it is a file that exists
if [[ -n $1 && -f $1 ]]; then
    TEST_BIN=$1
    shift
fi

# FYI on shift, moves all positional arguments left, 
# so make sure that we are handling each before shifting

# Check default or passed argument binary exists
if [[ ! -f $TEST_BIN ]]; then
    echo "Binary not found: $TEST_BIN"
    echo "Usage: $0 [--helgrind|--drd] [path_to_binary] [binary_args...]"
    exit 1
fi

echo "Running $TOOL on $TEST_BIN $@"

# Common base
VALGRIND_CMD=(valgrind --tool="$TOOL" --verbose)

# Add tool-specific flags
if [[ "$TOOL" == "memcheck" ]]; then
    VALGRIND_CMD+=(
        --leak-check=full
        --show-leak-kinds=all
        --track-origins=yes
    )
fi

# Run
"${VALGRIND_CMD[@]}" "$TEST_BIN" "$@"