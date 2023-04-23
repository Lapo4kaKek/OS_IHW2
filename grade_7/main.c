#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_PHILOSOPHERS 5
#define RUN_TIME 40
#define SEMAPHORE_PREFIX "/phil_sem_"
#define SHM_NAME "/phil_shm"

typedef struct philData {
    int fork_lft, fork_rgt;
    int eat_time;
} Philosopher;

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

    printf("Philosophers eating %d seconds\n", eat_time);

    // Создание именованных семафоров для вилок
    sem_t *forks[NUM_PHILOSOPHERS];
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        char sem_name[16];
        snprintf(sem_name, sizeof(sem_name), "%s%d", SEMAPHORE_PREFIX, i);
        forks[i] = sem_open(sem_name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
        if (forks[i] == SEM_FAILED) {
            perror("sem_open");
            return 1;
        }
    }

    // Создание разделяемой памяти для хранения структуры данных философов
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_fd, NUM_PHILOSOPHERS * sizeof(Philosopher)) == -1) {
        perror("ftruncate");
        return 1;
    }

    Philosopher *philosophers = mmap(NULL, NUM_PHILOSOPHERS * sizeof(Philosopher),
                                     PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (philosophers == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Заполнение структуры данных философов
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        philosophers[i].fork_lft = i;
        philosophers[i].fork_rgt = (i + 1) % NUM_PHILOSOPHERS;
        philosophers[i].eat_time = eat_time;
    }

    // Запуск процессов философов
        for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char idx_str[4];
            snprintf(idx_str, sizeof(idx_str), "%d", i);
            execl("./philosopher", "./philosopher", idx_str, NULL);
            perror("execl");
            exit(1);
        } else if (pid < 0) {
            perror("fork");
            return 1;
        }
    }

    // Ожидание перед завершением всех процессов-философов
    sleep(RUN_TIME);

    // Очистка ресурсов
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        char sem_name[16];
        snprintf(sem_name, sizeof(sem_name), "%s%d", SEMAPHORE_PREFIX, i);
        sem_close(forks[i]);
        sem_unlink(sem_name);
    }

    munmap(philosophers, NUM_PHILOSOPHERS * sizeof(Philosopher));
    close(shm_fd);
    shm_unlink(SHM_NAME);

    return 0;
}

