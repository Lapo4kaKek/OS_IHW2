#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define NUM_PHILOSOPHERS 5
#define SHM_SIZE 1024
#define KEY_PATH "/tmp/philosophers_key"

typedef struct {
    int eat_time;
} SharedData;

void philosopher(int index, int semid, SharedData *shared_data) {
    int left_fork = index;
    int right_fork = (index + 1) % NUM_PHILOSOPHERS;

    struct sembuf op_lock[2] = {
        {left_fork, -1, 0},
        {right_fork, -1, 0}
    };

    struct sembuf op_unlock[2] = {
        {left_fork, 1, 0},
        {right_fork, 1, 0}
    };

    while (1) {
        printf("Philosopher %d is thinking\n", index);
        sleep(1 + rand() % 8);

        printf("Philosopher %d is hungry\n", index);
        semop(semid, op_lock, 2);

        printf("Philosopher %d is eating for %d seconds\n", index, shared_data->eat_time);
        sleep(shared_data->eat_time);

        semop(semid, op_unlock, 2);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s philosopher_index\n", argv[0]);
        return 1;
    }

    int index = atoi(argv[1]);
    if (index < 0 || index >= NUM_PHILOSOPHERS) {
        printf("Incorrect input. Philosopher index must be between 0 and %d.\n", NUM_PHILOSOPHERS - 1);
        return 1;
    }

    key_t key = ftok(KEY_PATH, 1);
    if (key == -1) {
        perror("ftok");
        return 1;
    }

    int shmid = shmget(key, SHM_SIZE, 0644);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    SharedData *shared_data = (SharedData *) shmat(shmid, NULL, 0);

    int semid = semget(key, NUM_PHILOSOPHERS, 0644);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    philosopher(index, semid, shared_data);

    return 0;
}
