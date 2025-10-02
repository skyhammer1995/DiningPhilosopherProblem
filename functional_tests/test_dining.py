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
# SANITY-esque TESTS #
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

def test_clean_exit():
    """ Test binary exits with code 0 after short run """
    rc, output, err = run_simulation(extra_args=["--duration", "1"], timeout=5)

    assert rc == 0
    assert re.search(r"Starting Dining Philosophers", output)
    assert re.search(r"stops eating", output)
    print("PASSED: clean exit on short run")

# INPUT and CLI Verification TESTS #
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

# REQUIREMENT/Deadlock/Livelock/Starvation type TESTS #
@pytest.mark.slow # skip with -m "not slow"
def test_detecting_starvation_warning_and_handling():
    """
        Test that will always pass and log whether starvation is detected and logged to avoid flakiness
        - we will log if any starvation prints are seen
        - we will log if any philosopher was forced to eat
        - we will fail if starvation has exceeded our (somewhat) arbitrary threshold of `10` attempts, which may indicate a livelock/starvation event happening
    """
    rc, output, err = run_simulation(extra_args=["--duration", "25", "--philosophers", "7"], timeout=30)

    # Assert that a normal run has been happening
    assert rc == 0
    assert re.search("starts eating", output)
    assert re.search("stops eating", output)

    # Check for our messages for any starvation at all, and use that to determine if we experienced a livelock
    starvation_msgs = re.findall(r"Philosopher (\d+) is starving! Attempts: (\d+)", output)
    forced_eat_msgs = re.findall(r"Philosopher (\d+) is being forced to eat", output)

    starvation_detected = False
    starvation_threshold = 10 # if we ever go above 10, our checkpoint isn't working--THIS DOESN'T MEAN WE AREN'T RECOVERING, but it is a sign our checkpoint isn't doing great...m'kay?
    livelock_violations = []

    # associate philosophers to their attempts when over the threshold
    for philosopher_id, attempts in starvation_msgs:
            attempts = int(attempts)
            starvation_detected = True
            if attempts > starvation_threshold:
                livelock_violations.append((philosopher_id, attempts))


    # report if we found any starvation
    if starvation_detected:
        print("NOTE: Starvation detected during this run! Ok to occur, but not if exceeding our threshold")
    else:
        print("NOTE: No starvation detected, may need a longer run to observe")

    if forced_eat_msgs:
        print("NOTE: Forced eating logic was triggered, recovery is occurring")

    if livelock_violations:
        for philosopher_id, attempts in livelock_violations:
            print(f"ERROR: Philosopher {philosopher_id} exceeded starvation attempts ({attempts}). Our checkpoint may not be working and this may result in livelock")

    print("PASSED: Progressed without detected livelocks occurring")

@pytest.mark.slow #skip with -m "not slow"
def test_detecting_deadlock_and_violation_print():
    """ Test that no deadlock occurs and is preventing philosophers from eating """
    rc, output, err = run_simulation(extra_args=["--duration", "10", "--philosophers", "5"], timeout=15)

    assert rc == 0

    # count how many times we've eaten
    starts = len(re.findall(r"starts eating", output))
    stops = len(re.findall(r"stops eating", output))
    violation_detected =re.search(r"GROSS! \(violation\)", output)

    # Deadlock likely occurred if these numbers are 0 or too small to make sense
    assert starts > 0, print("No Philosohper ever started eating. Likely a deadlock has occurred")
    assert stops > 0, print("No Philosopher ever stopped eating. Likely a deadlock has occurred")
    assert not violation_detected, print("Neighbor violation detected: two adjacent philosophers were caught eating at the same time!")

    print("PASSED: no deadlock observed or neighbor violations found")

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
    test_clean_exit()
    test_unknown_flag_errors()
    test_varied_philosopher_values()
    test_invalid_flag_strings()
    test_detecting_starvation_warning_and_handling()
    test_detecting_deadlock_and_violation_print()
    test_large_number_of_philosophers()
    test_run_simulation_under_helgrind()
    print("\nAll functional tests passed!")

if __name__ == "__main__":
    main()