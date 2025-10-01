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

# Path to our source binary
BINARY = "./bin/diningPhilosophers"
DEFAULT_TIME = 20 # seconds
NUM_PHILOSOPHERS = 5

# SUB PROCESS helper #
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

# TESTS #
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

def test_unknown_flag_errors():
    """ Test that diningPhilosohpers fails gracefully on an unknown flag """
    rc, output, err = run_simulation(extra_args=["--InvalidFlag"], timeout=5)

    assert rc != 0
    assert re.search(r"Usage:", err)
    print ("PASSED: handled unknown flag")

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

    print("PASSED: philosopher was taken as input")

@pytest.mark.parametrize("flags", [["--duration", "abc"], ["--philosophers", "abc"]])
def test_invalid_flag_strings(flags):
    """ Parametrized test that we can pass multiple types of flags and the binary handles non-numeric values """
    rc, output, err = run_simulation(extra_args=flags, timeout=5)

    assert rc != 0
    assert re.search(r"(Invalid duration value:|Invalid philosopher value:)", err)
    print("PASSED: handled incorrect inputs")

def test_clean_exit():
    """ Test binary exits with code 0 after short run """
    rc, output, err = run_simulation(extra_args=["--duration", "1"], timeout=5)

    assert rc == 0
    assert re.search(r"Starting Dining Philosophers", output)
    assert re.search(r"stops eating", output)
    print("PASSED: clean exit on short run")

@pytest.mark.slow # skip with -m "not slow"
def test_detecting_starvation_warning_and_handling():
    """ Test that will always pass and log whether starvation is detected and logged """
    rc, output, err = run_simulation(extra_args=["--duration", "25", "--philosophers", "7"], timeout=30)

    # Check for our messages
    starvation_detected = re.search("is starving!", output)
    forced_eat_detected = re.search("is being forced to eat", output)

    # Always pass, but report if we found anything
    if starvation_detected:
        print("NOTE: Starvation detected during this run!")
    else:
        print("NOTE: No starvation detected, may need a longer run to observe")

    if forced_eat_detected:
        print("NOTE: Forced eating logic was triggered")

    # Assert that a normal run has been happening
    assert rc == 0
    assert re.search("starts eating", output)
    assert re.search("stops eating", output)
    print("PASSED: Handled detecting starvation, check report")

# @pytest.mark.slow #skip with -m "not slow"
# def test_detecting_deadlock_and_violation_print():


@pytest.mark.slow # skip with -m "not slow"
def test_large_number_of_philosophers():
    """ Test with 100 philosophers for 5 seconds to check scalability. """
    rc, output, err = run_simulation(extra_args=["--duration", "5", "--philosophers", "100"], timeout=10)

    assert rc == 0
    assert re.search(r"starts eating", output)
    print("PASSED: Handled large philosophers, and had clean exit")

@pytest.mark.slow # skip with -m "not slow"
def test_run_simulation_under_helgrind(timeout=40):
    """ Test Runs the valgrind::helgrind tool in order to detect any deadlocks with our dining philosophers """
    args = [
        "valgrind",
        "--tool=helgrind",
        "--error-exitcode=1", # This will cause helgrind to exit with a non-zero error if it finds/detects a deadlock
        "--ignore-thread-creation=yes",
        "./bin/diningPhilosophers",
        "--philosophers", "100",
        "--duration", "10"
    ]
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

    # Print Helgrind output
    stdout = result.stdout.decode()
    stderr = result.stderr.decode()
    print("PRINT STDOUT:")
    print(stdout)
    if stderr:
        print("PRINT STDERR:")
        print(stderr)

    # Effectively detect if we have any issues, primarily with deadlocks
    assert result.returncode == 0

# TEST RUNNER #
def main():
    # Run all functional tests
    test_all_philosophers_ate()
    test_all_return_to_thinking()
    test_unknown_flag_errors()
    test_varied_philosopher_values()
    test_invalid_flag_strings()
    test_clean_exit()
    test_detecting_starvation_warning_and_handling()
    test_large_number_of_philosophers()
    test_run_simulation_under_helgrind()
    print("\nAll functional tests passed!")

if __name__ == "__main__":
    main()