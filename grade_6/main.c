#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_PHILOSOPHERS 5
#define RUN_TIME 10
// проверка состояния программы
int running = 1;
// обработка сигнала для завершения программы 
void signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("I want to sleep, Goodbay\n");
        running = 0;
    }
}
///
/// Структура философа
/// fork_left/right - указатели на вилки
/// name, index, eat_time - имя, номер и время употребления пищи соответственно
typedef struct philData {
    int fork_left, fork_right;
    const char *name;
    int index;
    int eat_time;
} Philosopher;

///
/// Единая функция жизнедеятельности философов
/// 
void philosopher_func(Philosopher *phil) {
    struct sembuf first_fork_op, second_fork_op, first_fork_release, second_fork_release;

    first_fork_op = (struct sembuf){.sem_num = phil->index, .sem_op = -1, .sem_flg = 0};
    second_fork_op = (struct sembuf){.sem_num = (phil->index + 1) % NUM_PHILOSOPHERS, .sem_op = -1, .sem_flg = 0};
    first_fork_release = (struct sembuf){.sem_num = phil->index, .sem_op = 1, .sem_flg = 0};
    second_fork_release = (struct sembuf){.sem_num = (phil->index + 1) % NUM_PHILOSOPHERS, .sem_op = 1, .sem_flg = 0};

    while (running) {
        printf("%s is thinking\n", phil->name);
        sleep(1 + rand() % 8);

        printf("%s is hungry\n", phil->name);

        semop(phil->fork_left, &first_fork_op, 1);
        semop(phil->fork_right, &second_fork_op, 1);

        printf("%s is eating\n", phil->name);
        sleep(phil->eat_time);

        semop(phil->fork_right, &second_fork_release, 1);
        semop(phil->fork_left, &first_fork_release, 1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s eat_time\n", argv[0]);
        return 1;
    }
    // получаем время употребления пищи из командной строки
    int eat_time = atoi(argv[1]);
    if (eat_time < 1 || eat_time > 8) {
        printf("Incorrect input. Eat time must be between 1 and 8 seconds.\n");
        return 1;
    }
    // Установка обработчика сигнала
    signal(SIGINT, signal_handler);

    printf("Philosof eating %d seconds\n", eat_time);

    const char *name_list[] = {"Kant", "Guatma", "Russel", "Aristotle", "Bart"};
    // создаем философов
    Philosopher philosophers[NUM_PHILOSOPHERS];
    Philosopher *phil;
    int i;

    key_t key = ftok(".", 's');
    int semid = semget(key, NUM_PHILOSOPHERS, IPC_CREAT | 0666);
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        semctl(semid, i, SETVAL,        1);
    }
    // создаем разделяемую память для хранения философов
    int shmid = shmget(key, NUM_PHILOSOPHERS * sizeof(Philosopher), IPC_CREAT | 0666);
    Philosopher *shared_philosophers = (Philosopher *)shmat(shmid, NULL, 0);

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        phil = &shared_philosophers[i];
        phil->name = name_list[i];
        phil->index = i;
        phil->eat_time = eat_time;
        phil->fork_left = semid;
        phil->fork_right = semid;
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

    shmdt(shared_philosophers);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID, 0);

    return 0;
}
