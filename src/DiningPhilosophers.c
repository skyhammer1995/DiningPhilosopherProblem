#include <DiningPhilosophers.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

philosopher_t philosophers[NUM_PHILOSOPHERS];
pthread_mutex_t hashi[NUM_PHILOSOPHERS];
volatile int stop_flag = 0;

void *philosopher_routine(void *arg) {
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
                    p->violation_flag = VIOLATION;
                }
                
                printf("Philosopher %d starts eating\n", p->id);
                // "Eat" for 500 - 1500 ms
                sleep_ms(rand() % 1000 + 500);                  
                printf("Philosopher %d stops eating\n", p->id);

                // Reset
                p->state = THINKING;
                p->starvation_counter = 0;

                // RELEASE HASHI
                pthread_mutex_unlock(p->right_hashi);
                pthread_mutex_unlock(p->left_hashi);
            } else {
                // RIGHT HASHI IS UNAVAILABLE 
                // Put the left down and try later
                pthread_mutex_unlock(p->left_hashi);
                p->starvation_counter++;
            }
        } else {
            p->starvation_counter++;
        }
        // short delay before next attempt
        sleep_ms(rand() % 500 + 100);
    }

    /* C needs this to compile.
        We'll never hit this/it's unreachable.
        To clarify though, the return type is a void ptr--and we don't need to return the start address of anything, so we're good either way. */
    return NULL;
}

void sleep_ms(int millisec) {
    struct timespec ts; // needed by nanosleep

    ts.tv_sec = millisec/1000; // isolate the `seconds` component of the millisec value1
    ts.tv_nsec = (millisec % 1000) * 1000000L; // isolate the millisec portion of the value, and convert to nanoseconds
    nanosleep(&ts, NULL); // perform the sleep here
}

int init_hashi(void) {
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (pthread_mutex_init(&hashi[i], NULL) != 0) {
            for (int j = 0; j < i; j++) {
                pthread_mutex_destroy(&hashi[j]);
            }
            return -1;
        }
    }
    return 0;
}

void cleanup_hashi(void) {
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_destroy(&hashi[i]);
    }
}

void init_philosophers(void) {
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        philosophers[i].id = i;
        philosophers[i].state = THINKING;
        philosophers[i].left_hashi = &hashi[i];
        philosophers[i].right_hashi = &hashi[(i+1) % NUM_PHILOSOPHERS];
        philosophers[i].starvation_counter = 0;
        philosophers[i].violation_flag = 0;
    }
}

int start_philosophers(int runSeconds) {
    stop_flag = 0;

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (pthread_create(&philosophers[i].thread_id, NULL, philosopher_routine, &philosophers[i]) != 0) {
            return -1;
        }
    }

    sleep(runSeconds); // let the threads run for runSeconds time
    stop_flag = 1; // signal threads to exit their while loop

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_join(philosophers[i].thread_id, NULL);
    }

    return 0;
}



