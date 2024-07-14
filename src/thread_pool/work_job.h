#ifndef WORK_JOB_H
#define WORK_JOB_H

#include "thread_pool.h"
#include "work_queue.h"

struct work_job_t {

  struct work_steleaing_queue_t *local;
  struct thread_pool_t *pool;
  pthread_t thread;
};

struct work_job_t *creat_work_job(struct thread_pool_t *pool);
struct work_item_t *deque_job(struct work_job_t *work, int *missedSteal);
void destroy_work_job(struct work_job_t *work);

#endif
