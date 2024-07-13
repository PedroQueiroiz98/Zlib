#include "task.h"
#include "thread_pool.h"

struct task_t *create_task(void (*action)(void *), void (*continuation)(void *),
                           void *arg) {

  struct task_t *task = (struct task_t *)malloc(sizeof(struct task_t));
  assert(task != NULL);

  task->status = TASK_CREAED;
  task->action = action;
  task->continuation = continuation;
  task->arg = arg;

  return task;
}
static void run_task_internal(void *arg) {

  struct task_t *task = (struct task_t *)arg;
  task->action(task->arg);
  if (task->continuation != NULL) {
    struct task_t *cont = create_task(task->continuation, NULL, task->result);
    destroy_task(task);
    queue_work(cont->action, task->arg, true);
    cont->status = TASK_RUNNING;
  }
}
struct task_t *create_task_run(void (*action)(void *),
                               void (*continuation)(void *), void *arg) {

  struct task_t *task = create_task(action, continuation, arg);
  queue_work(run_task_internal, task, false);
  task->status = TASK_RUNNING;
  return task;
}
void destroy_task(struct task_t *task) { free(task); }