#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

#define PHILOSOPHERS 5
#define THINKING 0
#define HUNGRY 1
#define EATING 2

sem_t mutex;
sem_t S[PHILOSOPHERS];
int state[PHILOSOPHERS];
int philosophers[PHILOSOPHERS];

void *philosopher(void *num);
void take_forks(int);
void put_forks(int);
void test(int);

int main() {
    int i;
    pthread_t thread_id[PHILOSOPHERS];

    sem_init(&mutex, 0, 1);

    for (i = 0; i < PHILOSOPHERS; i++) {
        sem_init(&S[i], 0, 0);
        philosophers[i] = i;
    }

    for (i = 0; i < PHILOSOPHERS; i++) {
        pthread_create(&thread_id[i], NULL, philosopher, &philosophers[i]);
        printf("Философ %d садится за стол\n", i + 1);
    }

    for (i = 0; i < PHILOSOPHERS; i++) {
        pthread_join(thread_id[i], NULL);
    }

    return 0;
}

void *philosopher(void *num) {
    while (1) {
        int *i = num;
        sleep(1);
        take_forks(*i);
        sleep(0);
        put_forks(*i);
    }
}

void take_forks(int philosopher_number) {
    sem_wait(&mutex);
    state[philosopher_number] = HUNGRY;
    printf("Философ %d становится голодным\n", philosopher_number + 1);
    test(philosopher_number);
    sem_post(&mutex);
    sem_wait(&S[philosopher_number]);
    sleep(1);
}

void test(int philosopher_number) {
    if (state[philosopher_number] == HUNGRY &&
        state[(philosopher_number + 4) % PHILOSOPHERS] != EATING &&
        state[(philosopher_number + 1) % PHILOSOPHERS] != EATING) {
        state[philosopher_number] = EATING;
        sleep(2);
        printf("Философ %d берет вилки и начинает есть\n", philosopher_number + 1);
        sem_post(&S[philosopher_number]);
    }
}

void put_forks(int philosopher_number) {
    sem_wait(&mutex);
    state[philosopher_number] = THINKING;
    printf("Философ %d кладет вилки и начинает размышлять\n", philosopher_number + 1);
    test((philosopher_number + 4) % PHILOSOPHERS);
    test((philosopher_number + 1) % PHILOSOPHERS);
    sem_post(&mutex);
}
