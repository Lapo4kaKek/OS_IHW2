#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_PHILOSOPHERS 5

typedef struct philData {
    sem_t *fork_lft, *fork_rgt;
    const char *name;
    pthread_t thread;
} Philosopher;

int running = 1;

void *philosopher_func(void *p) {
    Philosopher *phil = (Philosopher *)p;
    sem_t *fork_lft, *fork_rgt;

    while (running) {
        printf("%s is thinking\n", phil->name);
        sleep(1 + rand() % 8);

        fork_lft = phil->fork_lft;
        fork_rgt = phil->fork_rgt;
        printf("%s is hungry\n", phil->name);

        sem_wait(fork_lft);
        sem_wait(fork_rgt);

        printf("%s is eating\n", phil->name);
        sleep(1 + rand() % 8);

        sem_post(fork_rgt);
        sem_post(fork_lft);
    }
    return NULL;
}
void *philosopher_func2(void *p) {
    Philosopher *phil = (Philosopher *)p;
    sem_t *first_fork, *second_fork;

    while (running) {
        printf("%s is thinking\n", phil->name);
        sleep(1 + rand() % 8);

        first_fork = (phil - philosophers
void ponder() {
    const char *name_list[] = {"Kant", "Guatma", "Russel", "Aristotle", "Bart"};
    sem_t forks[NUM_PHILOSOPHERS];
    Philosopher philosophers[NUM_PHILOSOPHERS];
    Philosopher *phil;
    int i;

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        sem_init(&forks[i], 0, 1);
    }

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        phil = &philosophers[i];
        phil->name = name_list[i];
        phil->fork_lft = &forks[i];
        phil->fork_rgt = &forks[(i + 1) % NUM_PHILOSOPHERS];
        pthread_create(&phil->thread, NULL, philosopher_func, phil);
    }

    sleep(40);
    running = 0;
    printf("cleanup time\n");

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        phil = &philosophers[i];
        pthread_join(phil->thread, NULL);
    }

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        sem_destroy(&forks[i]);
    }
}

int main() {
    ponder();
    return 0;
}
