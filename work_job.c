#include "work_job.h"
#include "fast_random.h"

#define ININIT_LOOP 1

struct fast_random_t fast_random;

void *work_job_run(void *arg) {

  init_fast_random(&fast_random, pthread_self());
  struct work_job_t *work = (struct work_job_t *)arg;
  notify_works_run(work->pool);
  struct thread_pool_t *pool = work->pool;
  struct work_queue_t *global_queue = pool->queue_global;

  int missedSteal = 0;
  printf("work is started thread_id :%ld\n", pthread_self());

  while (ININIT_LOOP) {

    if (pool->works_run != pool->works_count)
      continue;
    struct work_item_t *item = deque_job(work, &missedSteal);
    if (item == NULL && queue_is_empty(global_queue)) {
      await_work_itens(global_queue);
    }
    item->action(item->arg);
    free(item);
  }
}
struct work_item_t *deque_job(struct work_job_t *work, int *missedSteal) {

  struct work_queue_t *global_queue = work->pool->queue_global;
  struct thread_pool_t *pool = work->pool;
  struct work_item_t *call_back = NULL;

  if ((call_back = local_pop_core(work->local)) == NULL &&
      (call_back = pop(global_queue)) == NULL) {

    int c = pool->works_count;
    int max_index = c - 1;

    int i = fast_random_next(&fast_random, c);
    while (c > 0) {

      i = (i < max_index) ? i + 1 : 0;
      struct work_steleaing_queue_t *other_queue = pool->works[i]->local;
      if (other_queue != work->local && can_steal(other_queue)) {

        call_back = try_steal(other_queue, missedSteal);
        if (call_back != NULL) {
          break;
        }
      }
      c--;
    }
  }
  return call_back;
}

struct work_job_t *creat_work_job(struct thread_pool_t *pool) {

  struct work_job_t *work =
      (struct work_job_t *)malloc(sizeof(struct work_job_t));
  assert(work != NULL);

  work->local = create_stealing_queue();
  work->pool = pool;
  pthread_create(&work->thread, NULL, work_job_run, work);
  pthread_detach(work->thread);
  return work;
}

void destroy_work_job(struct work_job_t *work) {

  destroy_stealing_queue(work->local);
  pthread_cancel(work->thread);
  free(work);
}
