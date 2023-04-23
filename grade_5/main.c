#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

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
    sem_t *fork_lft, *fork_rgt;
    const char *name;
    int index;
    int eat_time;
} Philosopher;

void philosopher_func(Philosopher *phil) {
    sem_t *first_fork, *second_fork;

    while (running) {
        printf("%s is thinking\n", phil->name);
        sleep(1 + rand() % 8);

        first_fork = phil->index % 2 == 0 ? phil->fork_lft : phil->fork_rgt;
        second_fork = phil->index % 2 == 0 ? phil->fork_rgt : phil->fork_lft;
        printf("%s is hungry\n", phil->name);

        sem_wait(first_fork);
        sem_wait(second_fork);

        printf("%s is eating\n", phil->name);
        sleep(phil->eat_time);

        sem_post(second_fork);
        sem_post(first_fork);
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
    sem_t *forks[NUM_PHILOSOPHERS];
    Philosopher philosophers[NUM_PHILOSOPHERS];
    Philosopher *phil;
    int i;

    // Создание именованных семафоров
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        char sem_name[20];
        snprintf(sem_name, sizeof(sem_name), "/fork_sem_%d", i);
        forks[i] = sem_open(sem_name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
        sem_unlink(sem_name);
    }

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        phil = &philosophers[i];
        phil->name = name_list[i];
        phil->index = i;
        phil->eat_time = eat_time;
        phil->fork_lft = forks[i];
        phil->fork_rgt = forks[(i + 1) % NUM_PHILOSOPHERS];
        if (fork() == 0) {
            philosopher_func(phil);
            exit(0);                
        }
    }
    sleep(RUN_TIME);
    running = 0;
    printf("cleanup time\n");
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        wait(NULL);
    }
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        sem_close(forks[i]);
    }
    return 0;
}