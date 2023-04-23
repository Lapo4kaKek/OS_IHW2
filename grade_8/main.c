#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define COUNT 5 // Количество философов и вилок

#define NAME_SEM "/posix_semaphore1"   // Имя семафора для доступа к разделяемой памяти
#define NAME_ADMIN "/posix-admin1"      // Имя семафора для администрирования доступа
#define SHAR_OBJ "/posix-shar1"         // Имя объекта разделяемой памяти

typedef struct {
  int id;       // ID вилки
  int isUsingBy;// ID философа, который использует вилку (-1, если свободна)
} Fork;

typedef struct {
  int id;
  int count_of_meals; // Количество приемов пищи философом
  int isUsingFork;    // Флаг, использует ли философ вилку (0 - нет, 1 - да)
} Philosopher;

// Структура, хранящая информацию о состоянии стола и философов
typedef struct {
  Fork forks[COUNT];          // Массив всех вилок на столе
  int available;              // Количество свободных вилок
  Philosopher phil[COUNT];    // Массив философов
  pid_t philosopher_pid;      // PID философа
  pid_t output_pid;           // PID процесса, выводящего информацию
  int min_count_of_meal;      // Минимальное количество приемов пищи среди всех философов
  int count_of_meal_needed;   // Количество приемов пищи, которые должен сделать каждый философ
} shared_memory;

// Функция для вывода текущего состояния разделяемой памяти
void logger(shared_memory *buffer) {
  printf("\n=== Current state ===\n");
  for (int i = 0; i < COUNT; i++) {
    if (buffer->forks[i].isUsingBy == -1) {
      printf("fork %d is free\n", buffer->forks[i].id);
    } else {
      printf("fork %d is using by philosopher %d\n", buffer->forks[i].id, buffer->forks[i].isUsingBy);
    }
  }
  for (int i = 0; i < COUNT; i++) {
    printf("phil %d has ate %d times\n", i, buffer->phil[i].count_of_meals);
  }
}

// Функция-обработчик сигналов
void sigfunc(int sig, sem_t *sem, sem_t *admin, shared_memory *buffer) {
  if (sig != SIGINT && sig != SIGTERM) {
    return;
  }
  if (sig == SIGINT) {
    kill(buffer->philosopher_pid, SIGTERM);
    printf("Reader -> Writer\n");
  } else if (sig == SIGTERM) {
    printf("Writer -> Reader\n");
  }

  if (sem_close(sem) == -1) {
    perror("sem_close");
    exit(-1);
  }

  if (sem_close(admin) == -1) {
    perror("sem_close");
    exit(1);
  }

  // Удаляем семафоры
  sem_unlink(NAME_SEM);
  sem_unlink(NAME_ADMIN);

  exit(10);
}
int main(int argc, char *argv[]) {
  sem_t *sem;
  sem_t *admin;
  int fd;
  shared_memory *buffer;

  // Создаем разделяемую память
  fd = shm_open(SHAR_OBJ, O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    perror("shm_open");
    exit(EXIT_FAILURE);
  }

  ftruncate(fd, sizeof(shared_memory));

  buffer = (shared_memory *)mmap(NULL, sizeof(shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (buffer == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  printf("open shared memory\n");

  sem = sem_open(NAME_SEM, O_CREAT | O_EXCL, 0666, 1);
  if (sem == SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }
  admin = sem_open(NAME_ADMIN, O_CREAT, 0666, 0);
  if (admin == SEM_FAILED) {
    perror("sem_open");
    exit(1);
  }
  printf("make semaphores\n");
  for (int i = 0; i < COUNT; i++) {
    buffer->forks[i].id = i;
    buffer->forks[i].isUsingBy = -1;
    buffer->available = 5;
    buffer->phil[i].id = i;
    buffer->phil[i].count_of_meals = 0;
  }
  buffer->min_count_of_meal = 0;
  buffer->count_of_meal_needed = 3;
  if (argc == 2) {
    buffer->count_of_meal_needed = atoi(argv[1]);
  }
  buffer->output_pid = getpid();
  printf("init end\n");
  sem_post(admin);
  int ret = 0;
  sem_getvalue(admin, &ret);

  while (buffer->min_count_of_meal != buffer->count_of_meal_needed) {
    sleep(5);
    sem_wait(sem);
    logger(buffer);
    int min = 10000;
    for (int i = 0; i < COUNT; i++) {
      if (min > buffer->phil[i].count_of_meals) {
        min = buffer->phil[i].count_of_meals;
      }
    }
    if (min > buffer->min_count_of_meal) {
      buffer->min_count_of_meal = min;
    }
    sem_post(sem);
  }

  // Удаляем семафор и разделяемую память
  sem_unlink(NAME_SEM);
  sem_close(sem);
  shm_unlink(SHAR_OBJ);
  munmap(buffer, sizeof(shared_memory));
  close(fd);

  return 0;
}