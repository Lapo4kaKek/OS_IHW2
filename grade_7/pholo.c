#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define COUNT 5

typedef struct {
  int isUsingFork;
  int count_of_meals;
} Philosopher;

typedef struct {
  Philosopher phil[COUNT];
  int count_of_meal_needed;
} shared_memory;

typedef struct {
  shared_memory *buf;
  int id;
} A;

void funcPhil_prepareForEating(shared_memory *buf, int id) {
  if (buf->phil[id].isUsingFork == 0 && buf->phil[(id + 1) % COUNT].isUsingFork == 0) {
    buf->phil[id].isUsingFork = 1;
    buf->phil[(id + 1) % COUNT].isUsingFork = 1;
  }
}

void funcPhil_unreserveForks(shared_memory *buf, int id) {
  if (buf->phil[id].isUsingFork == 1 && buf->phil[(id + 1) % COUNT].isUsingFork == 1) {
    buf->phil[id].isUsingFork = 0;
    buf->phil[(id + 1) % COUNT].isUsingFork = 0;
  }
}

void funcPhil_eating(shared_memory *buf, int id) {
  buf->phil[id].count_of_meals++;
}

void funcPhil_thinking(shared_memory *buf, int id) {
  // Философ думает
}

void *startPhil(void *args) {
  A *ar = (A *)args;
  int id = ar->id;
  shared_memory *buff = ar->buf;

  while (buff->phil[id].count_of_meals < buff->count_of_meal_needed) {
    if (buff->phil[id].isUsingFork == 1) {
      printf("Philosopher %d is eating\n", id);
      funcPhil_eating(buff, id);
      sleep(rand() % 5 + 1);
      funcPhil_unreserveForks(buff, id);
    } else {
      printf("Philosopher %d is thinking\n", id);
      funcPhil_thinking(buff, id);
      sleep(rand() % 5 + 1);
      funcPhil_prepareForEating(buff, id);
    }
  }

  free(args); // Освобождаем выделенную память
  return NULL;
}

int main() {
  srand(time(NULL));
  pthread_t pt[COUNT];
  shared_memory buffer;

  buffer.count_of_meal_needed = 10;
  for (int i = 0; i < COUNT; i++) {
    buffer.phil[i].isUsingFork = 0;
    buffer.phil[i].count_of_meals = 0;
  }

  for (int i = 0; i < COUNT; i++) {
    A *arg;
    arg = (A *)malloc(sizeof(A));
    arg->buf = &buffer;
    arg->id = i;
    pthread_create(&pt[i], NULL, startPhil, arg);
  }

  for (int i = 0; i < COUNT; i++) {
    pthread_join(pt[i], NULL);
  }

  return 0;
}
