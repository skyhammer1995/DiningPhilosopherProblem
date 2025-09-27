#include <DiningPhilosophers.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

philosopher_t philosophers[NUM_PHILOSOPHERS];
pthread_mutex_t hashi[NUM_PHILOSOPHERS];
volatile int stop_flag = 0;

void *philosopher_routine(void *arg) {
    if (NUM_PHILOSOPHERS == 1) {
        return single_philosopher_routine(arg);
    }

    philosopher_t *p = (philosopher_t *)arg; // cast back to philosopher_t ptr
    int left_neighbor = (p->id + NUM_PHILOSOPHERS - 1) % NUM_PHILOSOPHERS;
    int right_neighbor = (p->id + 1) % NUM_PHILOSOPHERS;

    while (!stop_flag) {
        // THINK
        // "Think" for 500 - 1500 ms
        sleep_ms(rand() % 1000 + 500); 

        // ATTEMPTING TO EAT
        if (pthread_mutex_trylock(p->left_hashi) == 0) {         // Try to pick up left/personal hashi
            if (pthread_mutex_trylock(p->right_hashi) == 0) {    // Try to pick up right/neighbor's hashi
                // EAT
                p->state = EATING;
                if (philosophers[left_neighbor].state == EATING || philosophers[right_neighbor].state == EATING) {
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

    while (!stop_flag) {
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

int init_hashi(void) {
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        if (pthread_mutex_init(&hashi[i], NULL) != 0) {
            for (int j = 0; j < i; ++j) {
                pthread_mutex_destroy(&hashi[j]);
            }
            return -1;
        }
    }
    return 0;
}

void cleanup_hashi(void) {
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        pthread_mutex_destroy(&hashi[i]);
    }
}

void init_philosophers(void) {
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        philosophers[i].id = i;
        philosophers[i].state = THINKING;
        philosophers[i].left_hashi = &hashi[i];
        philosophers[i].right_hashi = &hashi[(i + 1) % NUM_PHILOSOPHERS];
        philosophers[i].starvation_counter = 0;
        philosophers[i].violation_flag = 0;
    }
}

int start_simulation(void) {
    // INPUT ERROR HANDLING
    if (NUM_PHILOSOPHERS <= 0) {
        fprintf(stderr, "Error: NUM_PHILOSOPHERS must be greater than 0!\n");
        return -1;
    }

    /* 
        The original spirit/semantics of Dining Philosophers say that a single philosopher should not be eating,
        but requirements are saying he should, and he has no neighbors to worry about. Any kind of edge case like this
        could have been clarified (like what ranges are possible/expected for this to scale up and down to) at the 
        High Level Requirements discussion stage.
    */
    if (NUM_PHILOSOPHERS == 1) {
        fprintf(stderr, "Notice: Running in single-philosopher mode.\n");
    }
    
    srand(time(NULL));

    // INITIALIZE OUR MUTEXES(hashi)
    if (init_hashi() != 0) {
        fprintf(stderr, "Error: initializing hashi\n");
        return -1;
    }
    
    // INITIALIZE THE PHILOSOPHER STRUCTS
    init_philosophers();

    printf("Starting Dining Philosophers...\n");
    
    // START OUR THREADS(philosophers)
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        int rc = pthread_create(&philosophers[i].thread_id, NULL, philosopher_routine, &philosophers[i]);
        if (rc != 0) {
            fprintf(stderr, "Error: pthread_create failed for philosopher %d: %s\n", i, strerror(rc));

            // Cleanup and join threads that are already created and started
            for (int j = 0; j < i; ++j) {
                pthread_cancel(philosophers[j].thread_id);
                pthread_join(philosophers[j].thread_id, NULL);
            }

            // cleanup already initialized mutexes
            cleanup_hashi();
            
            return -1;
        }
    }
    
    // JOIN THREADS, (technically this never should be reached, because we're endless)
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        pthread_join(philosophers[i].thread_id, NULL);
    }
    
    // DESTROY MUTEXES ON EXIT, (also technically unreachable in this program, since we are running endlessly)
    cleanup_hashi();

    return 0;
}

int start_philosophers_test(int runSeconds) {
    stop_flag = 0;

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        if (pthread_create(&philosophers[i].thread_id, NULL, philosopher_routine, &philosophers[i]) != 0) {
            return -1;
        }
    }

    sleep(runSeconds); // let the threads run for runSeconds time
    stop_flag = 1; // signal threads to exit their while loop

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        pthread_join(philosophers[i].thread_id, NULL);
    }

    return 0;
}
