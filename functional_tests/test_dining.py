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

    return result.returncode, stdout, stderr

def test_all_philosophers_ate():
    """ Test that all philosophers eventually ate at least once. """
    print("Running test: all philosophers ate at least once")
    rc, output, err = run_simulation(extra_args=["--duration", "15"]) # run for 15 seconds
    
    for i in range(NUM_PHILOSOPHERS):
        assert re.search(f"Philosopher {i} starts eating", output, re.IGNORECASE), \
            f"Philosopher {i} never ate"
    print("PASSED: all philosophers ate")

def test_all_return_to_thinking():
    """ Test that all philosophers stop eating/return to thinking at the end of the simulation. """
    print("Running test: all philosophers return to THINKING")
    NUM_PHILOSOPHERS = 10 # creates a local variable named NUM_PHILOSOPHERS (we shouldn't be updating the one further up)
    rc, output, err = run_simulation(extra_args=["--duration", "10", "--philosophers", str(NUM_PHILOSOPHERS)])
    
    for i in range(NUM_PHILOSOPHERS):
        assert re.search(f"Philosopher {i} stops eating", output, re.IGNORECASE), \
            f"Philosopher {i} did not stop eating"
    print("PASSED: all philosophers returned to THINKING")

@pytest.mark.parametrize("count", [-4, 0, 1, 9, 90])
def test_varied_philosopher_values(count):
    """ Parametrized test that we can pass multiple types of values for philosophers """
    print("Running test: Philosophers are passed as flag value")
    rc, output, err = run_simulation(extra_args=["--duration", "15", "--philosophers", str(count)])
    
    if count <= 0:
        assert re.search(f"Invalid philosopher value: {count}", err)
        assert rc != 0
    else:
        for i in range(count):
            assert re.search(f"Philosopher {i} starts eating", output, re.IGNORECASE), \
                f"Philosopher {i} never ate"
        assert rc == 0

    print ("PASSED: philosopher was taken as input")

def test_unknown_flag_errors():
    """ Test that diningPhilosohpers fails gracefully on an unknown flag """
    rc, output, err = run_simulation(extra_args=["--InvalidFlag"], timeout=5)

    assert rc != 0
    assert re.search(r"Usage:", err)
    print ("PASSED: handled unknown flag")

@pytest.mark.parametrize("flags", [["--duration", "abc"], ["--philosophers", "abc"]])
def test_invalid_flag_strings(flags):
    """ Parametrized test that we can pass multiple types of flags and the binary handles non-numeric values """
    rc, output, err = run_simulation(extra_args=flags, timeout=5)

    assert rc != 0
    assert re.search(r"(Invalid duration value:|Invalid philosopher value:)", err)
    print ("PASSED: handled incorrect inputs")

def test_clean_exit():
    """ Test binary exits with code 0 after short run """
    rc, output, err = run_simulation(extra_args=["--duration", "1"], timeout=5)

    assert rc == 0
    assert re.search(r"Starting Dining Philosophers", output)
    assert re.search(r"stops eating", output)
    print ("PASSED: clean exit on short run")

@pytest.mark.slow # skip with -m "not slow"
def test_large_number_of_philosophers():
    """Run with 50 philosophers for 5 seconds to check scalability."""
    rc, output, err = run_simulation(extra_args=["--duration", "10", "--philosophers", "50"], timeout=15)
    
    assert rc == 0
    assert re.search(r"starts eating", output)
    print("PASSED: Handled large philosophers, and had clean exit")

def main():
    # Run all functional tests
    test_all_philosophers_ate()
    test_all_return_to_thinking()
    test_varied_philosopher_values()
    test_unknown_flag_errors()
    test_invalid_flag_strings()
    test_clean_exit()
    print("\nAll functional tests passed!")

if __name__ == "__main__":
    main()