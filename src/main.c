/** Requirements:
    *  `Each philosopher should be simulated in their own thread.`
    *  `Each philosopher should wait for a random interval before starting to eat and then eat for a random interval before stopping.`
    *  `When a philosopher is eating, their neighbors may not eat.`
        -  `One option is that they may give up and try to eat later` 
        -  `Another option is to wait until the utensils they need are freed up`
    *  `There should be a way to tell when philosophers start or stop eating, such as a message log.`
    *  `The simulation should run endlessly until the program is stopped.`
*/

/**
 * @file main.c
 * @brief Simulation of the Dining Philosophers problem for SAS Assessment
 * @author Brandon Byrne
 */
#include <DiningPhilosophers.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (void) {
    srand(time(NULL));

    // INITIALIZE OUR MUTEXES(hashi)
    if (init_hashi() != 0) {
        fprintf(stderr, "Error: initializing hashi\n");
        return EXIT_FAILURE;
    }
    
    // INITIALIZE THE PHILOSOPHER STRUCTS
    init_philosophers();

    printf("Starting Dining Philosophers...\n");
    
    // START OUR THREADS(philosophers)
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        int rc = pthread_create(&philosophers[i].thread_id, NULL, philosopher_routine, &philosophers[i]);
        if (rc != 0) {
            fprintf(stderr, "Error: pthread_create failed for philosopher %d: %s\n", i, strerror(rc));

            // Cleanup and join threads that are already created and started
            for (int j = 0; j < i; j++) {
                pthread_cancel(philosophers[j].thread_id);
                pthread_join(philosophers[j].thread_id, NULL);
            }

            // cleanup already initialized mutexes
            for (int k = 0; k < NUM_PHILOSOPHERS; k++) {
                pthread_mutex_destroy(&hashi[k]);
            }
            
            return EXIT_FAILURE;
        }
    }
    
    // JOIN THREADS, (technically this never returns in this, since we're endless)
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_join(philosophers[i].thread_id, NULL);
    }
    
    // DESTROY MUTEXES ON EXIT, (also technically unreachable in this program, since we are running endlessly)
    cleanup_hashi();

    return EXIT_SUCCESS;
}