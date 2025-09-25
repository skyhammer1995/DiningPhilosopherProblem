#ifndef DININGPHILOSOPHERS_H
#define DININGPHILOSOPHERS_H

#include <pthread.h>

/** Total number of philosophers(threads) */
#define NUM_PHILOSOPHERS 100

/** Philosopher state */
typedef enum {
    THINKING = 0,
    EATING = 1
} philosopher_state_t;

/** Violation detection for tests */
typedef enum {
    OK = 0,
    VIOLATION =1
} violation_detection_t;

/** Philosopher struct encapsulates each thread's info */
typedef struct {
    int id;                                     // logging and easy identification
    pthread_mutex_t *left_hashi;                // keep left mutex
    pthread_mutex_t *right_hashi;               // keep right mutex
    philosopher_state_t state;                  // philosopher state used in testing mainly
    violation_detection_t violation_flag;       // violation detection flag for if eating while neighbor is eating
    int starvation_counter;                     // # of cycles without eating
    pthread_t thread_id;                        // thread identifier (don't use for math/only use for thread starting/joining etc.)
} philosopher_t;

/** Global data */
extern philosopher_t philosophers[NUM_PHILOSOPHERS];            // array holding the philosophers(struct type, includes threads)
extern pthread_mutex_t hashi[NUM_PHILOSOPHERS];                 // array holding the hashi(mutexes)
extern volatile int stop_flag;                                   // test enabler (stop the infinite loop)

/**
 * @brief The philosopher routine run by each thread
 * @param arg Pointer to the start address passed argument of the related philosopher
 * @return Always returns NULL
 * 
 * Each philosopher alternates between thinking and trying to eat.
 * Eating requires acquiring both left and right hashi (mutexes).
 * If a philosopher is unable to acquire both, they will release their held hashi, and will retry at a later time.
 */
void *philosopher_routine(void *arg);

/** HELPERS */
/**
 * @brief Helper function for sleeping the thread for a given value of milliseconds
 * @param millisec Number of milliseconds we wish to sleep for
 */
void sleep_ms(int millisec);
/** 
 * @brief Initialize all mutexes
 * @return 0 on success, non-zero on error
 */
int init_hashi(void);
/**
 * @brief cleanup all mutexes
 */
void cleanup_hashi(void);
/** 
 * @brief Initialize all philosopher structs
 * @return 0 on success, non-zero on error
 */
void init_philosophers(void);


/** TEST ENABLER FUNCTIONS */
/**
 * @brief Start philosopher threads for a finite duration
 * @param runSeconds Number of seconds to let philosophers run
 * @return 0 on success, non-zero on error
 * 
 * Sets stopFlag after runSeconds seconds, joins threads, then returns.
 */
int start_philosophers(int runSeconds);

#endif /* DININGPHILOSOPHERS_H */
