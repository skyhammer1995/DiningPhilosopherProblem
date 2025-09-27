#include <DiningPhilosophers.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

#include <stdbool.h>
#include <stdlib.h>

// cmocka API Documentation: https://api.cmocka.org/group__cmocka.html

/* Test: initialization and cleanup of mutexes*/
static void test_mutex_init(void **state) {
    (void)state;
    // init
    assert_int_equal(init_hashi(), 0);

    // cleanup
    cleanup_hashi();
}

/* Test: run the real philosophers for 5 seconds and verify are no violations */
static void test_philosophers_for_problems(void **state) {
    (void)state;
    // init
    assert_int_equal(init_hashi(), 0);
    init_philosophers();

    // run threads for 5 second (joins handled)
    assert_int_equal(start_philosophers_test(5), 0);
    
    // verify no violation for each thread
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        assert_int_not_equal(philosophers[i].violation_flag, VIOLATION);
        assert_in_range(philosophers[i].starvation_counter, 0, 10);
    }

    // cleanup
    cleanup_hashi();
}

/* Test: run philosopher routine and impose a violation */
static void test_philosophers_impose_violation(void **state) {
    (void)state;
    // init
    assert_int_equal(init_hashi(), 0);
    init_philosophers();

    // Impose the state that neighbors are eating
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        philosophers[i].state = EATING;        
    }

    // run threads for 1 second (joins handled)
    assert_int_equal(start_philosophers_test(1), 0);

    bool found_violation = false;

    // verify that simulated violation has occurred
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        if (philosophers[i].violation_flag == VIOLATION) {
            found_violation = true;
            break;
        }
    }

    assert_true(found_violation);

    // cleanup
    cleanup_hashi();
}

/* Test: directly call single_philosopher_routine */
static void test_single_philosopher_routine(void **state) {
    (void)state;
    // init
    pthread_mutex_t test_mutex;
    pthread_t t;
    pthread_mutex_init(&test_mutex, NULL);
    philosopher_t p = { .id = 0, 
                        .state = EATING, 
                        .left_hashi = &test_mutex };
    
    stop_flag = 0;  // make sure the while loop can run
    pthread_create(&t, NULL, single_philosopher_routine, &p);
    
    sleep_ms(1000); // 1s sleep -- this is technically non-deterministic and it would be better to mock (IDEAL SETUP)

    // stop the loop in case it's infinite
    stop_flag = 1;
    pthread_join(t, NULL);
    
    // ensure we went back to thinking
    assert_int_equal(p.state, THINKING);

    // cleanup
    pthread_mutex_destroy(&test_mutex);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_mutex_init),
        cmocka_unit_test(test_philosophers_for_problems),
        cmocka_unit_test(test_philosophers_impose_violation),
        cmocka_unit_test(test_single_philosopher_routine)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
