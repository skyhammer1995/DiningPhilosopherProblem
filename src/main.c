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

int main (int argc, char *argv[]) {
    int num_philosophers = 5; // default
    int duration_seconds = 0; // default: run indefinitely
    
    // CLI Parsing
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--philosophers") == 0 && i + 1 < argc) {
            num_philosophers = atoi(argv[++i]);
            if (num_philosophers <= 0) {
                num_philosophers = 5;
            } 
        } else if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) {
            duration_seconds = atoi(argv[++i]);
            if (duration_seconds < 0) {
                duration_seconds = 0;
            } 
        } else {
            fprintf(stderr, "Usage: %s [--philosophers N] [--duration SECONDS]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    simulation_t *sim = malloc(sizeof(simulation_t));
    if (!sim) {
        fprintf(stderr, "ERROR: Failed to allocate simulation\n");
        return EXIT_FAILURE;
    }

    sim->num_philosophers = num_philosophers;
    atomic_init(&sim->stop_flag, false);

    // Allocate the philosophers(thread) & hashi(mutex) arrays
    sim->hashi = malloc(sizeof(pthread_mutex_t) * sim->num_philosophers);
    if (!sim->hashi) {
        fprintf(stderr, "ERROR: Failed to allocate philosophers\n");
        free(sim->hashi);
        free(sim);

        return EXIT_FAILURE;
    }
    
    sim->philosophers = malloc(sizeof(philosopher_t) * sim->num_philosophers);
    if (!sim->philosophers) {
        fprintf(stderr, "ERROR: Failed to allocate philosophers\n");
        free(sim->philosophers);
        free(sim->hashi);
        free(sim);

        return EXIT_FAILURE;
    }

    int rc = start_simulation(sim, duration_seconds);

    // Free memory after simulation ends
    free(sim->philosophers);
    free(sim->hashi);
    free(sim);

    return rc;
}
