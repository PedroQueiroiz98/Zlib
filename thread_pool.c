#include "thread_pool.h"
#include <unistd.h>
struct thread_pool_t thread_pool;
once_t once;

static void int_thread_pool() {

  // struct thread_pool_t *pool =
  //  (struct thread_pool_t *)malloc(sizeof(struct thread_pool_t));
  // assert(pool != NULL);

  long works = 1;

  // sysconf(_SC_NPROCESSORS_ONLN) retorna o número de CPUs online disponíveis
  works = sysconf(_SC_NPROCESSORS_ONLN);

  printf("init thread pool whit total works %ld\n", works);

  struct thread_pool_t *pool = &thread_pool;

  pool->queue_global = create_work_queue();
  pool->works =
      (struct work_job_t **)malloc(sizeof(struct work_job_t *) * works);

  pool->works_count = works;

  pthread_mutex_init(&pool->lock, NULL);

  for (int i = 0; i < works; i++) {
    pool->works[i] = creat_work_job(pool);
  }
  // return pool;
}

static void queue_work_global(void (*call_back)(void *), void *arg) {

  struct thread_pool_t *pool = &thread_pool;
  push(pool->queue_global, call_back, arg);
}
static void queue_work_local(void (*call_back)(void *), void *arg) {

  struct thread_pool_t *pool = &thread_pool;
  for (int i = 0; i < pool->works_run; i++) {
    if (pthread_equal(pool->works[i]->thread, pthread_self())) {
      push_local(pool->works[i]->local, call_back, arg);
      return;
    }
  }
}
void queue_work(void (*call_back)(void *), void *arg, bool local) {

  if (pthread_once(&once, int_thread_pool))
    abort();
  if (local == true)
    queue_work_local(call_back, arg);
  else
    queue_work_global(call_back, arg);
}
void notify_works_run() {

  struct thread_pool_t *pool = &thread_pool;
  pthread_mutex_lock(&pool->lock);
  assert(pool->works_run <= pool->works_count);
  pool->works_run++;
  pthread_mutex_unlock(&pool->lock);
}
void destroy_destroy_thread_pool() {

  struct thread_pool_t *pool = &thread_pool;
  pthread_mutex_destroy(&pool->lock);
  destroy_work_queue(pool->queue_global);
  for (int i = 0; i < pool->works_count; i++) {
    destroy_work_job(pool->works[i]);
  }
  free(pool);
}