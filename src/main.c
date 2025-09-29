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
#include <errno.h>
#include <limits.h>

int main (int argc, char *argv[]) {
    // DEFAULTS
    int num_philosophers = 5; 
    int duration_seconds = 0; // default: run indefinitely
    
    // FOR INPUT VERIFICATION
    long tmp = 0;   // we will check for min and max to be safe to downcast to `int`
    char *endptr;   // to indicate if there's junk/trailing junk in our string
    errno = 0;      // to capture `strtol` error(s)

    // CLI Parsing
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--philosophers") == 0 && i + 1 < argc) {
            tmp = strtol(argv[++i], &endptr, /*base =*/ 10);
            if (errno != 0 || *endptr != '\0' || tmp <= 0 || tmp > INT_MAX) {
                fprintf(stderr, "Invalid philosopher value: %s\n", argv[i]);
                return EXIT_FAILURE;
            }
            num_philosophers = (int)tmp;
        } else if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) {
            tmp = strtol(argv[++i], &endptr, /*base =*/ 10);
            if (errno != 0 || *endptr != '\0' || tmp < 0 || tmp > INT_MAX) {
                fprintf(stderr, "Invalid duration value: %s\n", argv[i]);
                return EXIT_FAILURE;
            }
            duration_seconds = (int)tmp;
        } else {
            fprintf(stderr, "Usage: %s [--philosophers N] [--duration SECONDS]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Allocate the overall simulation encapsulation context
    simulation_t *sim = malloc(sizeof(simulation_t));
    if (!sim) {
        fprintf(stderr, "ERROR: Failed to allocate for simulation\n");
        return EXIT_FAILURE;
    }

    sim->num_philosophers = num_philosophers;
    atomic_init(&sim->stop_flag, false);

    // Allocate the hashi(mutex) array
    sim->hashi = malloc(sizeof(pthread_mutex_t) * sim->num_philosophers);
    if (!sim->hashi) {
        fprintf(stderr, "ERROR: Failed to allocate for hashi\n");
        free(sim->hashi);
        free(sim);

        return EXIT_FAILURE;
    }

    // Allocate the philosophers(thread) array
    sim->philosophers = malloc(sizeof(philosopher_t) * sim->num_philosophers);
    if (!sim->philosophers) {
        fprintf(stderr, "ERROR: Failed to allocate for philosophers\n");
        free(sim->philosophers);
        free(sim->hashi);
        free(sim);

        return EXIT_FAILURE;
    }

    // API Call
    int rc = start_simulation(sim, duration_seconds);

    // Free memory after simulation ends -- we want callers responsible for their memory management
    free(sim->philosophers);
    free(sim->hashi);
    free(sim);

    return rc;
}
