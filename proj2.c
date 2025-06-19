#include "proj2.h"

sem_t *shared_mem_sem = NULL;
FILE *output_file = NULL;
skier_t *skiers = NULL;
shared_data_t *shared_data = NULL;
int skiers_shm_id = -1;
int shared_data_shm_id = -1;

/**
 * @brief Create a named POSIX semaphore, compatible with WSL.
 * @param name Name of the semaphore.
 * @param value Initial value for the semaphore.
 * @return Pointer to the created semaphore, or NULL on failure.
 */
sem_t *create_semaphore(const char *name, int value) {
    sem_unlink(name);
    sem_t *sem = sem_open(name, O_CREAT | O_EXCL, 0644, value);
    if (sem == SEM_FAILED) {
        sem = sem_open(name, 0);
        if (sem == SEM_FAILED) {
            perror("Error creating/opening semaphore");
            return NULL;
        }
    }
    return sem;
}

/**
 * @brief Sleep for a given number of microseconds using nanosleep (WSL-safe).
 * @param microseconds Number of microseconds to sleep.
 */
void safe_sleep(int microseconds) {
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000;
    ts.tv_nsec = (microseconds % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

/**
 * @brief Initialize shared memory, semaphores, and output file.
 * @param config Simulation configuration.
 * @param skiers_shm_id Pointer to store skiers shared memory ID.
 * @param shared_data_shm_id Pointer to store shared data memory ID.
 * @return 0 on success, 1 on failure.
 */
int init_resources(config_t config, int *skiers_shm_id, int *shared_data_shm_id) {
    shared_mem_sem = create_semaphore(SEM_NAME, 1);
    if (shared_mem_sem == NULL) {
        perror("Error creating semaphore");
        return 1;
    }

    output_file = fopen("proj2.out", "w");
    if (output_file == NULL) {
        perror("Failed to open output file");
        sem_close(shared_mem_sem);
        sem_unlink(SEM_NAME);
        return 1;
    }

    key_t skiers_key = ftok("./proj2.c", 's');
    if (skiers_key == -1) {
        perror("Failed to create key for skiers shared memory");
        cleanup_resources();
        return 1;
    }

    *skiers_shm_id = shmget(skiers_key, sizeof(skier_t) * config.num_skiers, IPC_CREAT | 0666);
    if (*skiers_shm_id == -1) {
        fprintf(stderr, "Failed to create shared memory for skiers: %s (errno: %d)\n",
                strerror(errno), errno);
        cleanup_resources();
        return 1;
    }

    skiers = (skier_t *)shmat(*skiers_shm_id, NULL, 0);
    if (skiers == (void *)-1) {
        perror("Failed to attach shared memory for skiers");
        cleanup_resources();
        return 1;
    }

    // Initialize skier data
    for (int i = 0; i < config.num_skiers; i++) {
        skiers[i].id = i + 1;
        skiers[i].stop_id = rand_range(0, config.num_stops - 1);
        skiers[i].state = SKIER_BREAKFAST;
    }

    key_t shared_data_key = ftok("./proj2.c", 'd');
    if (shared_data_key == -1) {
        perror("Failed to create key for shared data memory");
        cleanup_resources();
        return 1;
    }

    *shared_data_shm_id = shmget(shared_data_key, sizeof(shared_data_t), IPC_CREAT | 0666);
    if (*shared_data_shm_id == -1) {
        fprintf(stderr, "Failed to create shared memory for shared data: %s (errno: %d)\n",
                strerror(errno), errno);
        cleanup_resources();
        return 1;
    }

    shared_data = (shared_data_t *)shmat(*shared_data_shm_id, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("Failed to attach shared memory for shared data");
        cleanup_resources();
        return 1;
    }

    // Initialize shared data
    shared_data->bus_stop_id = 0;
    shared_data->bus_occupied = 0;
    shared_data->action_count = 1;

    return 0;
}

/**
 * @brief Clean up shared resources: files, semaphores, and shared memory.
 */
void cleanup_resources() {
    if (output_file != NULL) {
        fclose(output_file);
        output_file = NULL;
    }

    if (shared_mem_sem != SEM_FAILED && shared_mem_sem != NULL) {
        sem_close(shared_mem_sem);
        sem_unlink(SEM_NAME);
        shared_mem_sem = NULL;
    }

    if (skiers != NULL && skiers != (void *)-1) {
        shmdt(skiers);
        skiers = NULL;
    }

    if (shared_data != NULL && shared_data != (void *)-1) {
        shmdt(shared_data);
        shared_data = NULL;
    }

    if (skiers_shm_id != -1) {
        shmctl(skiers_shm_id, IPC_RMID, NULL);
        skiers_shm_id = -1;
    }

    if (shared_data_shm_id != -1) {
        shmctl(shared_data_shm_id, IPC_RMID, NULL);
        shared_data_shm_id = -1;
    }
}

/**
 * @brief Signal handler for cleanup on termination signals.
 * @param signum Signal number.
 */
void signal_handler(int signum) {
    fprintf(stderr, "Caught signal %d, cleaning up and exiting.\n", signum);
    cleanup_resources();
    exit(1);
}

/**
 * @brief Main entry point for the skibus simulation.
 */
int main(int argc, char **argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    srand(time(NULL));
    config_t config = parse_config(argc, argv);

    if (init_resources(config, &skiers_shm_id, &shared_data_shm_id)) {
        fprintf(stderr, "Failed to initialize resources\n");
        cleanup_resources();
        return 1;
    }

    pid_t child_pid;
    for (int i = 0; i <= config.num_skiers; i++) {
        child_pid = fork();
        if (child_pid == 0) {
            srand(time(NULL) ^ getpid());
            if (i == 0) {
                skibus_process(skiers, shared_data, config);
            } else {
                skier_process(i - 1, skiers, shared_data, config);
            }
            exit(0);
        } else if (child_pid < 0) {
            perror("Error forking process");
            cleanup_resources();
            return 1;
        }
    }

    for (int i = 0; i <= config.num_skiers; i++) {
        wait(NULL);
    }

    cleanup_resources();
    return 0;
}

/**
 * @brief Process function for the skibus. Handles bus movement and skier boarding.
 */
void skibus_process(skier_t *skiers, shared_data_t *shared_data, config_t config) {
    print_message(MSG_BUS_START, shared_data, skiers, 0, 1);

    sem_wait(shared_mem_sem);
    shared_data->bus_occupied = 0;
    shared_data->bus_stop_id = 0;
    sem_post(shared_mem_sem);

    int stop = 0;
    int all_finished = 0;

    while (!all_finished) {
        safe_sleep(rand_range(0, config.max_bus_travel_time));
        print_message(MSG_BUS_ARRIVED, shared_data, skiers, stop, 1);

        // Board waiting skiers at this stop
        sem_wait(shared_mem_sem);
        for (int skier_id = 0; skier_id < config.num_skiers; skier_id++) {
            if (skiers[skier_id].state == SKIER_WAITING && skiers[skier_id].stop_id == stop) {
                if (shared_data->bus_occupied < config.bus_capacity) {
                    skiers[skier_id].state = SKIER_ONRIDE;
                    sem_post(shared_mem_sem);
                    print_message(MSG_SKIER_BOARDING, shared_data, skiers, skier_id, 1);
                    sem_wait(shared_mem_sem);
                    shared_data->bus_occupied++;
                }
            }
        }
        sem_post(shared_mem_sem);

        print_message(MSG_BUS_LEAVING, shared_data, skiers, stop, 1);
        stop = (stop + 1) % config.num_stops;

        // If a full round is completed, go to final stop
        if (stop == 0) {
            safe_sleep(rand_range(0, config.max_bus_travel_time));
            print_message(MSG_BUS_ARRIVED_FINAL, shared_data, skiers, 0, 1);

            // Unload skiers at final stop
            sem_wait(shared_mem_sem);
            for (int skier_id = 0; skier_id < config.num_skiers; skier_id++) {
                if (skiers[skier_id].state == SKIER_ONRIDE) {
                    skiers[skier_id].state = SKIER_FINISHED;
                    sem_post(shared_mem_sem);
                    print_message(MSG_SKIER_SKIING, shared_data, skiers, skier_id, 1);
                    sem_wait(shared_mem_sem);
                }
            }
            shared_data->bus_occupied = 0;

            // Check if all skiers are finished
            all_finished = 1;
            for (int skier_id = 0; skier_id < config.num_skiers; skier_id++) {
                if (skiers[skier_id].state != SKIER_FINISHED) {
                    all_finished = 0;
                    break;
                }
            }
            sem_post(shared_mem_sem);
            print_message(MSG_BUS_LEAVING_FINAL, shared_data, skiers, 0, 1);
        }
    }
    print_message(MSG_BUS_FINISH, shared_data, skiers, 0, 1);
}

/**
 * @brief Process function for a skier. Handles skier arrival and waiting.
 */
void skier_process(int skier_id, skier_t *skiers, shared_data_t *shared_data, config_t config) {
    print_message(MSG_SKIER_START, shared_data, skiers, skier_id, 1);
    safe_sleep(rand_range(0, config.max_skier_wait_time));
    sem_wait(shared_mem_sem);
    skiers[skier_id].state = SKIER_WAITING;
    sem_post(shared_mem_sem);
    print_message(MSG_SKIER_ARRIVED, shared_data, skiers, skier_id, 1);
    // Skier waits for bus, handled by skibus_process
}

/**
 * @brief Parse command-line arguments into simulation configuration.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Parsed config_t structure.
 */
config_t parse_config(int argc, char **argv) {
    if (argc != NUM_ARGS) {
        fprintf(stderr, "Invalid number of arguments. Expected 5, got %d.\n", argc - 1);
        exit(EXIT_FAILURE);
    }

    config_t config;
    config.num_skiers = atoi(argv[1]);
    config.num_stops = atoi(argv[2]);
    config.bus_capacity = atoi(argv[3]);
    config.max_skier_wait_time = atoi(argv[4]);
    config.max_bus_travel_time = atoi(argv[5]);

    // Validate parameters
    if (config.num_skiers < 1 || config.num_skiers > MAX_SKIERS) {
        fprintf(stderr, "Number of skiers must be between 1 and %d.\n", MAX_SKIERS);
        exit(EXIT_FAILURE);
    }
    if (config.num_stops < 1 || config.num_stops > MAX_STOPS) {
        fprintf(stderr, "Number of stops must be between 1 and %d.\n", MAX_STOPS);
        exit(EXIT_FAILURE);
    }
    if (config.bus_capacity < 10 || config.bus_capacity > 100) {
        fprintf(stderr, "Skibus capacity must be between 10 and 100.\n");
        exit(EXIT_FAILURE);
    }
    if (config.max_skier_wait_time < 0 || config.max_skier_wait_time > 10000) {
        fprintf(stderr, "Maximum skier wait time must be between 0 and 10000 microseconds.\n");
        exit(EXIT_FAILURE);
    }
    if (config.max_bus_travel_time < 0 || config.max_bus_travel_time > 1000) {
        fprintf(stderr, "Maximum bus travel time must be between 0 and 1000 microseconds.\n");
        exit(EXIT_FAILURE);
    }
    return config;
}

/**
 * @brief Generate a random integer in the range [min, max].
 * @param min Minimum value.
 * @param max Maximum value.
 * @return Random integer in the specified range.
 */
int rand_range(int min, int max) {
    if (min == max) {
        return min;
    }
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    int range = max - min + 1;
    int remainder = RAND_MAX % range;
    int value;
    do {
        value = rand();
    } while (value >= RAND_MAX - remainder);
    return value % range + min;
}

/**
 * @brief Print a formatted simulation message to stdout and output file.
 * @param msg_type Type of message.
 * @param shared_data Pointer to shared data.
 * @param skiers Pointer to skier array.
 * @param index Index for skier or stop.
 * @param use_semaphore Whether to use semaphore for synchronization.
 */
void print_message(message_type_t msg_type, shared_data_t *shared_data, skier_t *skiers, int index, int use_semaphore) {
    char buffer[256];
    if (use_semaphore) {
        sem_wait(shared_mem_sem);
    }
    sprintf(buffer, "%d: ", shared_data->action_count);
    switch (msg_type) {
        case MSG_SKIER_START:
        case MSG_SKIER_ARRIVED:
        case MSG_SKIER_BOARDING:
        case MSG_SKIER_SKIING:
            sprintf(buffer + strlen(buffer), "L %d: ", skiers[index].id);
            break;
        case MSG_BUS_START:
        case MSG_BUS_ARRIVED:
        case MSG_BUS_LEAVING:
        case MSG_BUS_ARRIVED_FINAL:
        case MSG_BUS_LEAVING_FINAL:
        case MSG_BUS_FINISH:
            sprintf(buffer + strlen(buffer), "BUS: ");
            break;
        default:
            break;
    }
    switch (msg_type) {
        case MSG_SKIER_START:
            strcat(buffer, "started\n");
            break;
        case MSG_BUS_START:
            strcat(buffer, "started\n");
            break;
        case MSG_SKIER_ARRIVED:
            sprintf(buffer + strlen(buffer), "arrived to %d\n", skiers[index].stop_id + 1);
            break;
        case MSG_SKIER_BOARDING:
            strcat(buffer, "boarding\n");
            break;
        case MSG_SKIER_SKIING:
            strcat(buffer, "going to ski\n");
            break;
        case MSG_BUS_ARRIVED:
            sprintf(buffer + strlen(buffer), "arrived to %d\n", index + 1);
            break;
        case MSG_BUS_LEAVING:
            sprintf(buffer + strlen(buffer), "leaving %d\n", index + 1);
            break;
        case MSG_BUS_ARRIVED_FINAL:
            strcat(buffer, "arrived to final\n");
            break;
        case MSG_BUS_LEAVING_FINAL:
            strcat(buffer, "leaving final\n");
            break;
        case MSG_BUS_FINISH:
            strcat(buffer, "finish\n");
            break;
        default:
            fprintf(stderr, "Unexpected message type: %d\n", msg_type);
            break;
    }
    printf("%s", buffer);
    fprintf(output_file, "%s", buffer);
    fflush(output_file);
    shared_data->action_count++;
    if (use_semaphore) {
        sem_post(shared_mem_sem);
    }
}