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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s eat_time\n", argv[0]);
        return 1;
    }

    int eat_time = atoi(argv[1]);
    if (eat_time < 1 || eat_time > 8) {
        printf("Incorrect input. Eat time must be between 1 and 8 seconds.\n");
        return 1;
    }

    key_t key = ftok(KEY_PATH, 1);
    if (key == -1) {
        perror("ftok");
        return 1;
    }

    int shmid = shmget(key, SHM_SIZE, 0644 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    SharedData *shared_data = (SharedData *) shmat(shmid, NULL, 0);
    shared_data->eat_time = eat_time;

    int semid = semget(key, NUM_PHILOSOPHERS, IPC_CREAT | 0644);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        semctl(semid, i, SETVAL, 1);
    }

    printf("Start philosophers. Press any key to stop.\n");
    getchar();

    // Cleanup
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
    }

    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl");
    }

    return 0;
}
