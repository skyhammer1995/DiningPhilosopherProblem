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

#include <stdlib.h>

int main (void) {
    return start_simulation();
}
