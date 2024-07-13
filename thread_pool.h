#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "work_job.h"
#include <pthread.h>
#include <stdbool.h>

typedef pthread_once_t once_t;

struct thread_pool_t {

  struct work_queue_t *queue_global;
  struct work_job_t **works;
  int works_count;
  int works_run;
  pthread_mutex_t lock;
};

// void int_thread_pool(int works);
void queue_work(void (*call_back)(void *), void *arg, bool local);

void notify_works_run();
void destroy_destroy_thread_pool();

#endif