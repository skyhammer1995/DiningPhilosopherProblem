#include <DiningPhilosophers.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

void *philosopher_routine(void *arg) {
    philosopher_t *p = (philosopher_t *)arg; // cast back to philosopher_t ptr
    simulation_t *sim = p->sim;

    if (sim->num_philosophers == 1) {
        return single_philosopher_routine(arg);
    }

    int left_neighbor = (p->id + sim->num_philosophers - 1) % sim->num_philosophers;
    int right_neighbor = (p->id + 1) % sim->num_philosophers;

    while (!atomic_load(&sim->stop_flag)) {
        // THINK
        // "Think" for 500 - 1500 ms
        sleep_ms(rand() % 1000 + 500); 

        // ATTEMPTING TO EAT
        if (pthread_mutex_trylock(p->left_hashi) == 0) {         // Try to pick up left/personal hashi
            if (pthread_mutex_trylock(p->right_hashi) == 0) {    // Try to pick up right/neighbor's hashi
                // EAT
                p->state = EATING;
                // This technically should not happen since we'd need to have the mutexes available to get here. 
                if (sim->philosophers[left_neighbor].state == EATING || sim->philosophers[right_neighbor].state == EATING) {
                    printf("Philosopher %d ate with his hands, GROSS! (violation)\n", p->id);
                    p->violation_flag = VIOLATION;
                }
                
                printf("Philosopher %d starts eating\n", p->id);
                sleep_ms(rand() % 1000 + 500);                  
                printf("Philosopher %d stops eating\n", p->id);

                // RESET
                p->state = THINKING;
                p->starvation_counter = 0;

                // RELEASE HASHI
                pthread_mutex_unlock(p->right_hashi);
                pthread_mutex_unlock(p->left_hashi);
            } else {
                // RIGHT HASHI IS UNAVAILABLE 
                // Put the left down and try later
                pthread_mutex_unlock(p->left_hashi);
                ++p->starvation_counter;
            }
        } else {
            // NO HASHI ARE AVAILABLE
            ++p->starvation_counter;
        }
        // short delay before next attempt
        sleep_ms(rand() % 500 + 100);
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
        p->state = EATING;
        printf("Philosopher %d starts eating (single-philosopher mode)\n", p->id);
        sleep_ms(rand() % 1000 + 500);
        printf("Philosopher %d stops eating (single-philosopher mode)\n", p->id);
        
        // RESET
        p->state = THINKING;
        
        // RELEASE SINGLE HASHI
        pthread_mutex_unlock(p->left_hashi);
    }

    return NULL;
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

    for (int i = 0; i < sim->num_philosophers; ++i) {
        sim->philosophers[i].id = i;
        sim->philosophers[i].state = THINKING;
        sim->philosophers[i].left_hashi = &sim->hashi[i];
        sim->philosophers[i].right_hashi = &sim->hashi[(i + 1) % sim->num_philosophers];
        sim->philosophers[i].starvation_counter = 0;
        sim->philosophers[i].violation_flag = 0;
        sim->philosophers[i].sim = sim;
    }

    return 0;
}

int start_simulation(simulation_t *sim, int duration_seconds) {
    // INPUT ERROR HANDLING
    if (sim->num_philosophers <= 0) {
        fprintf(stderr, "Error: num_philosophers must be greater than 0!\n");
        return -1;
    }

    /* 
        The original spirit/semantics of Dining Philosophers say that a single philosopher should not be eating,
        but requirements are saying he should, and he has no neighbors to worry about. Any kind of edge case like this
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
    
    // INITIALIZE THE PHILOSOPHER STRUCTS
    if (init_philosophers(sim) != 0) {
        fprintf(stderr, "Error: initializing philosophers!\n");
        return -1;
    }

    printf("Starting Dining Philosophers...\n");
    
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
        printf("Run for duration: %d seconds\n", duration_seconds);
        sleep_ms(duration_seconds * 1000); // convert to milliseconds
        atomic_store(&sim->stop_flag, true);
    } else {
        printf("Running until stopped\n");
        while (!atomic_load(&sim->stop_flag)) {
            // Sleep a little so we don't burn CPU, but still wake up to check the flag
            sleep_ms(500);        
        }
    }
    
    // JOIN THREADS, (technically this never should be reached, because we're endless)
    for (int i = 0; i < sim->num_philosophers; ++i) {
        pthread_join(sim->philosophers[i].thread_id, NULL);
    }
    
    // DESTROY MUTEXES ON EXIT, (also technically unreachable in this program, since we are running endlessly)
    cleanup_hashi(sim);

    return 0;
}
