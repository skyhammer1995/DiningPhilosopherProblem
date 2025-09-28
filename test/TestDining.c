#include <DiningPhilosophers.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

// cmocka API Documentation: https://api.cmocka.org/group__cmocka.html

/*============== Impromptu Wrapper ==============*/
struct sim_args {
    simulation_t *sim;
    int duration;
};

// Need to pass stuff otherwise we end up with garbage values, so a non-routine API isn't going to work, so we wrap!
static void *start_indefinite_wrapper(void *arg) {
    struct sim_args *a = arg;
    start_simulation(a->sim, a->duration);
    return NULL;
}

/*============== Test Fixtures ==============*/
static int setup_simulation(void **state) {
    simulation_t *sim = malloc(sizeof(simulation_t));
    assert_non_null(sim);

    sim->num_philosophers = 10;
    atomic_init(&sim->stop_flag, false);

    // Allocate arrays for hashi and philosophers
    sim->hashi = malloc(sizeof(pthread_mutex_t) * sim->num_philosophers);
    assert_non_null(sim->hashi);

    sim->philosophers = malloc(sizeof(philosopher_t) * sim->num_philosophers);
    assert_non_null(sim->philosophers);

    // Initialize
    assert_int_equal(init_hashi(sim), 0);
    assert_int_equal(init_philosophers(sim), 0);

    *state = sim;
    return 0;
}

static int teardown(void **state) {
    simulation_t *sim = *(simulation_t **)state;

    cleanup_hashi(sim);

    free(sim->philosophers);
    free(sim->hashi);
    free(sim);
    return 0;
}

/*============== Tests ==============*/
static void test_init_hashi_and_philosophers(void **state) {
    simulation_t *sim = * (simulation_t **)state;
    assert_int_equal(sim->num_philosophers, 10);
    assert_non_null(sim->hashi);
    assert_non_null(sim->philosophers);

    for (int i = 0; i < sim->num_philosophers; ++i) {
        philosopher_t *p = &sim->philosophers[i];
        assert_int_equal(p->id, i);
        assert_ptr_equal(p->left_hashi, &sim->hashi[i]);
        assert_ptr_equal(p->right_hashi, &sim->hashi[(i + 1) % sim->num_philosophers]);
        assert_int_equal(atomic_load(&p->state), THINKING);
        assert_int_equal(p->violation_flag, OK);
    }
}

static void test_cleanup_hashi_with_invalid_hashi(void **state) {
    simulation_t *sim = * (simulation_t **)state;

    // Keep track of allocated memory so we can free it later
    pthread_mutex_t *temp = sim->hashi; // save our pointer

    // Modify hashi to invalid value/nullptr
    sim->hashi = NULL;

    cleanup_hashi(sim);

    // Restore
    sim->hashi = temp;
}

static void test_single_philosopher_mode(void **state) {
    simulation_t *sim = * (simulation_t **)state;

    // update num of philosophers to 1
    sim->num_philosophers = 1;

    // Start_simulation blocks until duration elapses
    assert_int_equal(start_simulation(sim, 2), 0);
}

static void test_start_simulation_with_duration(void **state) {
    simulation_t *sim = * (simulation_t **)state;

    // Start_simulation blocks until duration elapses
    assert_int_equal(start_simulation(sim, 2), 0);
    // Ensure the flag flipped
    assert_true(atomic_load(&sim->stop_flag));
}

static void test_start_simulation_with_invalid_num_philosophers(void **state) {
    simulation_t *sim = * (simulation_t **)state;

    // Keep track of allocated memory so we can free it later
    int temp = sim->num_philosophers;

    // Modify num_philosopher to invalid value
    sim->num_philosophers = 0;

    assert_int_not_equal(start_simulation(sim, 2), 0);

    // Restore
    sim->num_philosophers = temp;
}

static void test_start_simulation_with_invalid_hashi(void **state) {
    simulation_t *sim = * (simulation_t **)state;

    // Keep track of allocated memory so we can free it later
    pthread_mutex_t *temp = sim->hashi; // save our pointer

    // Modify hashi to invalid value/nullptr
    sim->hashi = NULL;

    assert_int_not_equal(start_simulation(sim, 2), 0);

    // Restore
    sim->hashi = temp;
}

static void test_start_simulation_with_invalid_philosohpers(void **state) {
    simulation_t *sim = * (simulation_t **)state;

    // Keep track of allocated memory so we can free it later
    philosopher_t *temp = sim->philosophers; // save our pointer

    // Modify philosophers to invalid value/nullptr
    sim->philosophers = NULL;

    assert_int_not_equal(start_simulation(sim, 2), 0);

    // Restore
    sim->philosophers = temp;
}

static void test_start_indefinite_simulation_and_enable_stop_flag(void **state) {
    simulation_t *sim = * (simulation_t **)state;

    pthread_t thread_id;
    struct sim_args args = {sim, 0};

    // I'll note, casting the start_simulation return type was a pain
    int rc = pthread_create(&thread_id, NULL, start_indefinite_wrapper, &args);
    assert_int_equal(rc, 0);

    // Give time for the thread to run the simulation briefly before we flip the flag
    sleep(1);

    // Flip flag to break the loop
    atomic_store(&sim->stop_flag, true);

    // Wait for the sim_thread to exit
    pthread_join(thread_id, NULL);

    // Ensure the stop flag was set
    assert_true(atomic_load(&sim->stop_flag));
}

/*============== Test Runner ==============*/
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_init_hashi_and_philosophers, setup_simulation, teardown),
        cmocka_unit_test_setup_teardown(test_cleanup_hashi_with_invalid_hashi, setup_simulation, teardown),
        cmocka_unit_test_setup_teardown(test_single_philosopher_mode, setup_simulation, teardown),
        cmocka_unit_test_setup_teardown(test_start_simulation_with_duration, setup_simulation, teardown),
        cmocka_unit_test_setup_teardown(test_start_simulation_with_invalid_num_philosophers, setup_simulation, teardown),
        cmocka_unit_test_setup_teardown(test_start_simulation_with_invalid_hashi, setup_simulation, teardown),
        cmocka_unit_test_setup_teardown(test_start_simulation_with_invalid_philosohpers, setup_simulation, teardown),
        cmocka_unit_test_setup_teardown(test_start_indefinite_simulation_and_enable_stop_flag, setup_simulation, teardown)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
