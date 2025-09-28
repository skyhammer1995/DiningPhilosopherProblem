#!/usr/bin/env python3
"""
Functional tests for Dining Philosophers simulation.
Requires compiled binary at ./bin/diningPhilosophers
"""

import subprocess
import sys
import re

# Path to your binary
BINARY = "./bin/diningPhilosophers"
# Default simulation time in seconds
DEFAULT_TIME = 5
NUM_PHILOSOPHERS = 5

def run_simulation(timeout=DEFAULT_TIME, extra_args=None):
    """
    Run the simulation binary with optional arguments.
    Returns stdout as string.
    """
    args = [BINARY]
    if extra_args:
        args.extend(extra_args)

    try:
        result = subprocess.run(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout
        )
    except subprocess.TimeoutExpired:
        print(f"Simulation timed out after {timeout} seconds")
        sys.exit(1)

    stdout = result.stdout.decode()
    stderr = result.stderr.decode()

    if stderr:
        print("STDERR from binary:")
        print(stderr)

    return stdout

def test_all_philosophers_ate():
    """
    Test that all philosophers eventually ate at least once.
    """
    print("Running test: all philosophers ate at least once")
    output = run_simulation()
    for i in range(NUM_PHILOSOPHERS):
        assert re.search(f"Philosopher {i} starts eating", output, re.IGNORECASE), \
            f"Philosopher {i} never ate"
    print("PASS: all philosophers ate")

def test_violation_detected():
    """
    Test that a violation occurs when imposed (neighbors already eating).
    """
    print("Running test: violation occurs when neighbors eating")
    # Assume your binary can optionally start with violation imposed via command line
    output = run_simulation(extra_args=["--impose-violation"])
    assert re.search("violation", output, re.IGNORECASE), "No violation detected"
    print("PASS: violation detected")

def test_all_return_to_thinking():
    """
    Test that all philosophers return to THINKING at the end of the simulation.
    """
    print("Running test: all philosophers return to THINKING")
    output = run_simulation()
    for i in range(NUM_PHILOSOPHERS):
        assert re.search(f"Philosopher {i} thinking", output, re.IGNORECASE), \
            f"Philosopher {i} did not return to THINKING"
    print("PASS: all philosophers returned to THINKING")

def main():
    # Run all functional tests
    test_all_philosophers_ate()
    test_violation_detected()
    test_all_return_to_thinking()
    print("\nAll functional tests passed!")

if __name__ == "__main__":
    main()