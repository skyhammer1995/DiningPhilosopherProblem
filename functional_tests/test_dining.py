#!/usr/bin/env python3
"""
Functional tests for Dining Philosophers simulation.
Requires compiled binary at ./bin/diningPhilosophers
"""

import subprocess
import sys
import re
import pytest

# pytest documentation ref: https://docs.pytest.org/en/stable/how-to/usage.html

# Path to your binary
BINARY = "./bin/diningPhilosophers"
DEFAULT_TIME = 20 # seconds
NUM_PHILOSOPHERS = 5

def run_simulation(timeout=DEFAULT_TIME, extra_args=None):
    """
    Helper: Runs the diningPhilosophers binary and capture its stdout.
    timeout: we give a default timeout time to ensure we never hang
    extra_args: if we want to pass optional flags like `--duration` or `--philosophers`
    """
    args = [BINARY]
    if extra_args:
        args.extend(extra_args)

    try:
        result = subprocess.run(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired:
        print(f"Simulation timed out after {timeout} seconds")
        sys.exit(1)

    stdout = result.stdout.decode()
    stderr = result.stderr.decode()

    # If we have an error, print the failure from diningPhilosophers
    if stderr:
        print("STDERR from binary:")
        print(stderr)

    return stdout

# this is not a great test, non-deterministic and relies on timing to cover all states
def test_all_philosophers_ate():
    """ Test that all philosophers eventually ate at least once. """
    print("Running test: all philosophers ate at least once")
    output = run_simulation(extra_args=["--duration", "15"]) # run for 15 seconds
    
    for i in range(NUM_PHILOSOPHERS):
        assert re.search(f"Philosopher {i} starts eating", output, re.IGNORECASE), \
            f"Philosopher {i} never ate"
    print("PASS: all philosophers ate")

def test_all_return_to_thinking():
    """ Test that all philosophers stop eating/return to thinking at the end of the simulation. """
    print("Running test: all philosophers return to THINKING")
    NUM_PHILOSOPHERS = 20 # creates a local variable named NUM_PHILOSOPHERS (we shouldn't be updating the one further up)
    output = run_simulation(extra_args=["--duration", "10", "--philosophers", str(NUM_PHILOSOPHERS)])
    
    for i in range(NUM_PHILOSOPHERS):
        assert re.search(f"Philosopher {i} stops eating", output, re.IGNORECASE), \
            f"Philosopher {i} did not stop eating"
    print("PASS: all philosophers returned to THINKING")

@pytest.mark.parametrize("count", [-4, 0, 1, 9, 90])
def test_varied_philosopher_values(count):
    """ Parametrized test that we can pass multiple types of values for philosophers """
    print("Running test: Philosophers are passed as flag value")
    output = run_simulation(extra_args=["--duration", "10", "--philosophers", str(count)])
    
    if count <= 0:
        # diningPhilosophers will use default value 5 if the passed philosopher value is <= 0
        range_value = 5
    else:
        range_value = count
    
    for i in range(range_value):
        assert re.search(f"Philosopher {i} starts eating", output, re.IGNORECASE), \
            f"Philosopher {i} never ate"
    print ("PASSED: philosopher was taken as input")

def main():
    # Run all functional tests
    test_all_philosophers_ate()
    test_all_return_to_thinking()
    test_varied_philosopher_values()
    print("\nAll functional tests passed!")

if __name__ == "__main__":
    main()