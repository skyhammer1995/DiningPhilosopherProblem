#include <DiningPhilosophers.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

/**
 * Update: after helgrind analysis, we weren't using global lock order for our hashi.
 *  i.e: Always lock the lower-index hashi first (99th thread would have 99 and 0 for left and right,
 *       don't do left then right, rather lowest to highest)
 *       I will try to stick with trylock to avoid blocking
 *
 * After pytest helgrind'ing, I'm realizing that the data races we're creating with our printfs need to be handled.
 * I will implement a safe_print helper
 */

void *philosopher_routine(void *arg) {
    philosopher_t *p = (philosopher_t *)arg; // cast back to philosopher_t ptr
    simulation_t *sim = p->sim;

    if (sim->num_philosophers == 1) {
        return single_philosopher_routine(arg);
    }

    // Get global indexes for the mutexes (based of the thread ids)
    const int left_idx = p->id;
    const int right_idx =(p->id + 1) % sim->num_philosophers;

    // Update for global ordering/always attempt the lower indexed hashi first
    pthread_mutex_t *first_hashi = (left_idx < right_idx) ? p->left_hashi : p->right_hashi;
    pthread_mutex_t *second_hashi = (left_idx < right_idx) ? p->right_hashi : p->left_hashi;

    int left_neighbor = (p->id + sim->num_philosophers - 1) % sim->num_philosophers;
    int right_neighbor = (p->id + 1) % sim->num_philosophers;

    while (!atomic_load(&sim->stop_flag)) {
        // THINK
        // "Think" for 500 - 1500 ms
        sleep_ms(rand() % 1000 + 500);

        // ATTEMPTING TO EAT
        if (pthread_mutex_trylock(first_hashi) == 0) {         // Try to pick up smallest indexed hashi
            if (pthread_mutex_trylock(second_hashi) == 0) {    // Try to pick up the other possible hashi
                // EAT
                atomic_store(&p->state, EATING);
                // This technically should not happen since we'd need to have the mutexes available to get here.
                if (atomic_load(&sim->philosophers[left_neighbor].state) == EATING ||
                    atomic_load(&sim->philosophers[right_neighbor].state) == EATING) {
                    safe_printf(sim, "Philosopher %d ate with his hands, GROSS! (violation)\n", p->id);
                    p->violation_flag = VIOLATION;
                }

                safe_printf(sim, "Philosopher %d starts eating\n", p->id);
                sleep_ms(rand() % 1000 + 500);
                safe_printf(sim, "Philosopher %d stops eating\n", p->id);

                // RESET
                atomic_store(&p->state, THINKING);
                p->starvation_counter = 0;

                // RELEASE HASHI
                pthread_mutex_unlock(second_hashi);
                pthread_mutex_unlock(first_hashi);
            } else {
                // SECOND HASHI IS UNAVAILABLE
                // Put the left down and try later
                pthread_mutex_unlock(first_hashi);
                ++p->starvation_counter;
            }
        } else {
            // NO HASHI ARE AVAILABLE
            ++p->starvation_counter;
        }

        // Handle starving philosophers checkpoint
        if (p->starvation_counter >= 10) {
            safe_printf(sim, "Philosopher %d is starving! Attempts: %d\n", p->id, p->starvation_counter);
            // We can do forced acquisition in here or some priority track,
            // or just increase our back off timer to help with further desyncing below

            // Small randomized sleep to reduce contention
            sleep_ms(rand() % 50 + 50);
            pthread_mutex_lock(first_hashi);
            pthread_mutex_lock(second_hashi);

            safe_printf(sim, "Philosopher %d is being forced to eat\n", p->id);
            sleep_ms(rand() % 1000 + 500);
            safe_printf(sim, "Philosopher %d no longer being forced to eat\n", p->id);

            pthread_mutex_unlock(second_hashi);
            pthread_mutex_unlock(first_hashi);

            p->starvation_counter = 0;
        }

        // short delay before next attempt
        sleep_ms(rand() % 100 + 50);
    }

    // Technically never hit, but needed
    return NULL;
}

void *single_philosopher_routine(void *arg) {
    philosopher_t *p = (philosopher_t *)arg;
    simulation_t *sim = p->sim;

    while (!atomic_load(&sim->stop_flag)) {
        // THINK
        sleep_ms(rand() % 1000 + 500);
        pthread_mutex_trylock(p->left_hashi); // only possible hashi (we could technically just use lock)

        // EATING
        atomic_store(&p->state, EATING);
        safe_printf(sim, "Philosopher %d starts eating (single-philosopher mode)\n", p->id);
        sleep_ms(rand() % 1000 + 500);
        safe_printf(sim, "Philosopher %d stops eating (single-philosopher mode)\n", p->id);

        // RESET
        atomic_store(&p->state, THINKING);

        // RELEASE SINGLE HASHI
        pthread_mutex_unlock(p->left_hashi);
    }

    return NULL;
}

void safe_printf(simulation_t *sim, const char *format, ...) {
    va_list args;           // C having variadic functions is wild to me, but I guess they all follow this pattern
    va_start(args, format); // access our variable arguments

    pthread_mutex_lock(&sim->thread_safe_print_mutex);
    vprintf(format, args);  // print formatted output
    fflush(stdout);         // ensure the output is flushed immediately
    pthread_mutex_unlock(&sim->thread_safe_print_mutex);

    va_end(args); // clean up the list when we're done (I don't know how this works yet, and I haven't dove deep into it)
}

void sleep_ms(int millisec) {
    struct timespec ts; // needed by nanosleep

    ts.tv_sec = millisec/1000; // isolate the `seconds` component of the millisec value
    ts.tv_nsec = (millisec % 1000) * 1000000L; // isolate the millisec portion of the value, and convert to nanoseconds
    nanosleep(&ts, NULL); // perform the sleep here
}

int init_hashi(simulation_t *sim) {
    if (!sim->hashi) {
        return -1;
    }

    for (int i = 0; i < sim->num_philosophers; ++i) {
        if (pthread_mutex_init(&sim->hashi[i], NULL) != 0) {
            fprintf(stderr, "Failed to init hashi %d\n", i);

            for (int j = 0; j < i; ++j) {
                pthread_mutex_destroy(&sim->hashi[j]);
            }

            return -1;
        }
    }
    return 0;
}

void cleanup_hashi(simulation_t *sim) {
    if (!sim->hashi) {
        return;
    }

    for (int i = 0; i < sim->num_philosophers; ++i) {
        pthread_mutex_destroy(&sim->hashi[i]);
    }
}

int init_philosophers(simulation_t *sim) {
    if (!sim->philosophers || !sim->hashi) {
        return -1;
    }

    // Set pthread thread_id member when creating the threads in start_simulation()
    for (int i = 0; i < sim->num_philosophers; ++i) {
        sim->philosophers[i].id = i;
        atomic_store(&sim->philosophers[i].state, THINKING);
        sim->philosophers[i].left_hashi = &sim->hashi[i];
        sim->philosophers[i].right_hashi = &sim->hashi[(i + 1) % sim->num_philosophers];
        sim->philosophers[i].starvation_counter = 0;
        sim->philosophers[i].violation_flag = 0;
        sim->philosophers[i].sim = sim;
    }

    return 0;
}

int start_simulation(simulation_t *sim, int duration_seconds) {
    // INPUT ERROR HANDLING -- we shouldn't hit this now
    if (sim->num_philosophers <= 0) {
        fprintf(stderr, "Error: num_philosophers must be greater than 0!\n");
        return -1;
    }

    /*
        The original spirit/semantics of Dining Philosophers say that a single philosopher should not be eating,
        but requirements are saying he should (or are at least ambiguous enough to say he should),
        and he has no neighbors to worry about. Any kind of edge case like this
        could have been clarified (like what ranges are possible/expected for this to scale up and down to) at the
        High Level Requirements discussion stage.
    */
    if (sim->num_philosophers == 1) {
        fprintf(stderr, "Notice: Running in single-philosopher mode.\n");
    }

    // SEED TIME FOR RAND()
    srand(time(NULL));

    // INITIALIZE OUR MUTEXES(hashi)
    if (init_hashi(sim) != 0) {
        fprintf(stderr, "Error: initializing hashi!\n");
        return -1;
    }

    // INITIALIZE thread_safe_print_mutex
    if (pthread_mutex_init(&sim->thread_safe_print_mutex, NULL) != 0) {
        fprintf(stderr, "Error: failed to initialize thread_safe_print_mutex!\n");
        return -1;
    }

    // INITIALIZE THE PHILOSOPHER STRUCTS
    if (init_philosophers(sim) != 0) {
        fprintf(stderr, "Error: initializing philosophers!\n");
        return -1;
    }

    safe_printf(sim, "Starting Dining Philosophers...\n");

    // START OUR THREADS(philosophers)
    for (int i = 0; i < sim->num_philosophers; ++i) {
        int rc = pthread_create(&sim->philosophers[i].thread_id, NULL, philosopher_routine, &sim->philosophers[i]);
        if (rc != 0) {
            fprintf(stderr, "Error: pthread_create failed for philosopher %d: %s\n", i, strerror(rc));
            atomic_store(&sim->stop_flag, true);
            // Cleanup and join threads that are already created and started
            for (int j = 0; j < i; ++j) {
                pthread_cancel(sim->philosophers[j].thread_id);
                pthread_join(sim->philosophers[j].thread_id, NULL);
            }

            // cleanup already initialized mutexes
            cleanup_hashi(sim);
            return -1;
        }
    }

    if (duration_seconds > 0) {
        safe_printf(sim, "Run for duration: %d seconds\n", duration_seconds);
        sleep_ms(duration_seconds * 1000); // convert to milliseconds
        atomic_store(&sim->stop_flag, true);
    } else {
        safe_printf(sim, "Running until stopped\n");
        while (!atomic_load(&sim->stop_flag)) {
            // Sleep a little so we don't burn CPU, but still wake up to check the flag
            sleep_ms(500);
        }
    }

    // JOIN THREADS, (technically this never should be reached, because we're endless)
    for (int i = 0; i < sim->num_philosophers; ++i) {
        pthread_join(sim->philosophers[i].thread_id, NULL);
    }

    // DESTROY thread_safe_print_mutex
    pthread_mutex_destroy(&sim->thread_safe_print_mutex);

    // DESTROY MUTEXES(hashi) ON EXIT, (also technically unreachable in this program, since we are running endlessly)
    cleanup_hashi(sim);

    return 0;
}
