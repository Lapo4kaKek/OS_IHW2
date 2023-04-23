#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUM_PHILOSOPHERS 5
#define RUN_TIME 40

int running = 1;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("I want to sleep, Goodbay\n");
        running = 0;
    }
}

typedef struct philData {
    int fork_lft, fork_rgt;
    const char *name;
    int index;
    int eat_time;
    int semid;
} Philosopher;

void philosopher_func(Philosopher *phil) {
    int sem_buf_lft = 0, sem_buf_rgt = 0;
    struct sembuf op_lock[2] = {
        {0, -1, SEM_UNDO},
        {1, -1, SEM_UNDO}
    };
    struct sembuf op_unlock[2] = {
        {0, 1, SEM_UNDO},
        {1, 1, SEM_UNDO}
    };

    while (running) {
        printf("%s is thinking\n", phil->name);
        sleep(1 + rand() % 8);

        printf("%s is hungry\n", phil->name);

        op_lock[0].sem_num = phil->fork_lft;
        op_lock[1].sem_num = phil->fork_rgt;
        semop(phil->semid, op_lock, 2);

        printf("%s is eating\n", phil->name);
        sleep(phil->eat_time);

        op_unlock[0].sem_num = phil->fork_lft;
        op_unlock[1].sem_num = phil->fork_rgt;
        semop(phil->semid, op_unlock, 2);
    }
}

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

    // Установка обработчика сигнала
    signal(SIGINT, signal_handler);
    printf("Philosof eating %d seconds\n", eat_time);

    const char *name_list[] = {"Kant", "Guatma", "Russel", "Aristotle", "Bart"};
    Philosopher philosophers[NUM_PHILOSOPHERS];
    int semid;

    key_t key = ftok("/tmp", 'p');
    semid = semget(key, NUM_PHILOSOPHERS, 0666 | IPC_CREAT);

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        semctl(semid, i, SETVAL, 1);
    }
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        Philosopher *phil = &philosophers[i];
        phil->name = name_list[i];
        phil->index = i;
        phil->eat_time = eat_time;
        phil->fork_lft = i;
        phil->fork_rgt = (i + 1) % NUM_PHILOSOPHERS;
        phil->semid = semid;
        if (fork() == 0) {
            philosopher_func(phil);
            exit(0);
        }
    }
    sleep(RUN_TIME);
    running = 0;
    printf("cleanup time\n");

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        wait(NULL);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        semctl(semid, i, IPC_RMID);
    }

    return 0;
}
