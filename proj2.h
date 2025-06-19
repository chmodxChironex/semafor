#ifndef _SKIBUS_SIM_H_
#define _SKIBUS_SIM_H_

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <errno.h>
#include <signal.h>

#define MAX_SKIERS 20000           // Maximum number of skiers supported
#define MAX_STOPS 10               // Maximum number of bus stops
#define NUM_ARGS 6                 // Number of expected command-line arguments
#define SEM_NAME "/xklimam00_skibus_sem" // Name for the POSIX semaphore

/**
 * @brief Configuration parameters for the simulation.
 */
typedef struct {
    int num_skiers;               // Number of skiers
    int num_stops;                // Number of bus stops
    int bus_capacity;             // Capacity of the bus
    int max_travel_time;          // Unused, kept for compatibility
    int max_skier_wait_time;      // Maximum time a skier waits before arriving at the stop
    int max_bus_travel_time;      // Maximum time for bus travel between stops
} config_t;

/**
 * @brief Possible states of a skier during the simulation.
 */
typedef enum {
    SKIER_BREAKFAST,              // Skier is having breakfast
    SKIER_WAITING,                // Skier is waiting at the stop
    SKIER_ONRIDE,                 // Skier is on the bus
    SKIER_FINISHED                // Skier has finished the ride
} skier_state_t;

/**
 * @brief Structure representing a skier.
 */
typedef struct {
    int id;                       // Unique skier ID
    int stop_id;                  // Assigned stop for the skier
    skier_state_t state;          // Current state of the skier
} skier_t;

/**
 * @brief Shared data structure for inter-process communication.
 */
typedef struct {
    int bus_stop_id;              // Current bus stop ID
    int bus_occupied;             // Number of skiers currently on the bus
    int action_count;             // Counter for printed actions
} shared_data_t;

/**
 * @brief Types of messages for logging simulation events.
 */
typedef enum {
    MSG_SKIER_START,
    MSG_SKIER_ARRIVED,
    MSG_SKIER_BOARDING,
    MSG_SKIER_SKIING,
    MSG_BUS_START,
    MSG_BUS_ARRIVED,
    MSG_BUS_LEAVING,
    MSG_BUS_ARRIVED_FINAL,
    MSG_BUS_LEAVING_FINAL,
    MSG_BUS_FINISH
} message_type_t;

// Global variables for shared resources
extern sem_t *shared_mem_sem;
extern FILE *output_file;
extern skier_t *skiers;
extern shared_data_t *shared_data;
extern int skiers_shm_id;
extern int shared_data_shm_id;

// Function prototypes
void skier_process(int skier_id, skier_t *skiers, shared_data_t *shared_data, config_t config);
void skibus_process(skier_t *skiers, shared_data_t *shared_data, config_t config);
void print_message(message_type_t msg_type, shared_data_t *shared_data, skier_t *skiers, int index, int use_semaphore);
config_t parse_config(int argc, char **argv);
int rand_range(int min, int max);
void cleanup_resources();
void signal_handler(int signum);
int init_resources(config_t config, int *skiers_shm_id, int *shared_data_shm_id);
sem_t *create_semaphore(const char *name, int value);
void safe_sleep(int microseconds);

#endif // _SKIBUS_SIM_H_