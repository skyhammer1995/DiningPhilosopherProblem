#include <DiningPhilosophers.h>

#include <stdarg.h>
#include <setjmp.h>
#include <stddef.h>
#include <cmocka.h>

#include <pthread.h>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>

/* Test: mutex init/cleanup works */
static void test_mutex_init(void **state) {
    (void)state;
    assert_int_equal(init_mutexes(), 0);
    cleanup_mutexes();
}

/* Test: run the real philosophers for 1 second */
static void test_philosophers_run(void **state) {
    (void)state;
    assert_int_equal(init_mutexes(), 0);
    /* start_philosophers should create threads, sleep runSeconds,
       set stopFlag, join threads and return 0 on success */
    assert_int_equal(start_philosophers(1), 0);
    // verify that we did not find a violation
    assert_int_not_equal(test_violationDetected, VIOLATION);
    cleanup_mutexes();
}

/* Test: run the real philosophers for 5 seconds */
static void test_long_run(void **state) {
    (void)state;
    assert_int_equal(init_mutexes(), 0);
    assert_int_equal(start_philosophers(5), 0); // longer test
    assert_int_not_equal(test_violationDetected, VIOLATION);
    for (int i = 0; i < NUM_PHILOSOPHERS; i++)
        assert_int_equal(test_eatState[i], THINKING); // verify that no one is left eating when we're done
    cleanup_mutexes();
}



int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_mutex_init),
        cmocka_unit_test(test_philosophers_run),
        cmocka_unit_test(test_long_run)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
