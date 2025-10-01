#ifndef DININGPHILOSOPHERS_H
#define DININGPHILOSOPHERS_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdarg.h> // need this for the `...` variadic in safe_printf's signature

/*============== TYPEDEFS ==============*/
/** Philosopher state for tests */
typedef enum {
    THINKING = 0,
    EATING = 1
} philosopher_state_t;

/** Violation detection for tests */
typedef enum {
    OK = 0,
    VIOLATION = 1
} violation_detection_t;

// forward declaration
typedef struct simulation simulation_t;

/** Philosopher struct encapsulates each thread's info */
typedef struct {
    int id;                                     // logging and easy identification
    pthread_mutex_t *left_hashi;                // keep left mutex
    pthread_mutex_t *right_hashi;               // keep right mutex
    _Atomic philosopher_state_t state;          // philosopher state used in testing mainly. can be checked by other threads, so atomic
    violation_detection_t violation_flag;       // violation detection flag for if eating while neighbor is eating
    int starvation_counter;                     // number of cycles without eating
    pthread_t thread_id;                        // thread identifier (don't use for math/only use for thread starting/joining etc.)
    simulation_t *sim;                          // points back to the overall simulation context
} philosopher_t;

/** Simulation context -- full encapsulation, no global variables in this version */
struct simulation {
    int num_philosophers;
    philosopher_t *philosophers;
    pthread_mutex_t *hashi;
    atomic_bool stop_flag;   // atomic for cross-thread safety
    pthread_mutex_t thread_safe_print_mutex;
};

/*============== MAIN ROUTINES ==============*/
/**
 * @brief The philosopher routine run by each thread
 * @param arg Pointer to the start address passed argument of the related philosopher
 * @return void*: Normally returns NULL, except when invoking the single_philosopher_routine
 *
 * Each philosopher alternates between thinking and trying to eat.
 * Eating requires acquiring both left and right hashi (mutexes).
 * If a philosopher is unable to acquire both, they will release their held hashi, and will retry at a later time.
 */
void *philosopher_routine(void *arg);
/**
 * @brief The philosopher routine run by a single thread when there is only one philosopher
 * @param arg Pointer to the start address passed argument of the only philosopher
 * @returns void*: always returns NULL
 */
void *single_philosopher_routine(void *arg);

/*============== HELPERS ==============*/
/**
 * @brief Thread safe printf wrapper
 * @param sim Pointer to the simulation context to access the mutex object
 * @param format printf-style format string
 * @param ... additional arguments corresponding to format specifiers (I didn't know this existed until this was a problem, and I found an implementable solution on StackOverflow)
 *
 * To satisfy helgrind and pytest, and not have data races between the different philosopher threads using the printf buffers
 */
void safe_printf(simulation_t *sim, const char *format, ...);
/**
 * @brief Helper function for sleeping the thread for a given value of milliseconds
 * @param millisec Number of milliseconds we wish to sleep for
 */
void sleep_ms(int millisec);
/**
 * @brief Initialize all mutexes
 * @param sim Pointer to the simulation context
 * @return int: 0 on success, non-zero on error
 */
int init_hashi(simulation_t *sim);
/**
 * @brief cleanup all mutexes
 * @param sim Pointer to the simulation context
 */
void cleanup_hashi(simulation_t *sim);
/**
 * @brief Initialize all philosopher structs
 * @param sim Pointer to the simulation context
 */
int init_philosophers(simulation_t *sim);

/*============== MAIN API ==============*/
/**
 * @brief Start the endless dining philosophers simulation.
 * @param sim Pointer to the simulation context
 * @param duration_seconds the amount of time that user wishes to run the philosophers for (0 is default and infinite)
 *
 * Initializes mutexes, creates the philosopher threads (joins and cleans up, but never executes)
 * Blocks forever until the process is killed.
 *
 * @return int: 0 on success, non-zero error
 */
int start_simulation(simulation_t *sim, int duration_seconds);

#endif /* DININGPHILOSOPHERS_H */
