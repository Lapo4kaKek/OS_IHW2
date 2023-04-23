#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define NUM_PHILOSOPHERS 5
#define SEMAPHORE_PREFIX "/phil_sem_"
#define SHM_NAME "/phil_shm"

typedef struct philData {
    int fork_lft, fork_rgt;
    int eat_time;
} Philosopher;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s philosopher_index\n", argv[0]);
        return 1;
    }

    int index = atoi(argv[1]);

    // Открытие именованных семафоров для вилок
    sem_t *forks[NUM_PHILOSOPHERS];
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        char sem_name[16];
        snprintf(sem_name, sizeof(sem_name), "%s%d", SEMAPHORE_PREFIX, i);
        forks[i] = sem_open(sem_name, 0);
        if (forks[i] == SEM_FAILED) {
            perror("sem_open");
            return 1;
        }
    }

    // Открытие разделяемой памяти для структуры данных философов
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    Philosopher *philosophers = mmap(NULL, NUM_PHILOSOPHERS * sizeof(Philosopher),
                                     PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (philosophers == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    Philosopher *phil = &philosophers[index];

    while (1) {
        // Философ думает
        printf("Philosopher %d is thinking\n", index);
        sleep(2);

        // Философ голоден и пытается взять вилки
                printf("Philosopher %d is hungry\n", index);
        sem_wait(forks[phil->fork_lft]);
        sem_wait(forks[phil->fork_rgt]);

        // Философ ест
        printf("Philosopher %d is eating\n", index);
        sleep(phil->eat_time);

        // Философ кладет вилки
        sem_post(forks[phil->fork_lft]);
        sem_post(forks[phil->fork_rgt]);
    }

    // Освобождение ресурсов
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        sem_close(forks[i]);
    }

    munmap(philosophers, NUM_PHILOSOPHERS * sizeof(Philosopher));
    close(shm_fd);

    return 0;
}

