#ifndef TASK_H
#define TASK_H

enum task_status_t {
  TASK_CREAED,
  TASK_RUNNING,
  TASK_SUCCESS,
  TASK_FAILED,
};

struct task_t {

  enum task_status_t status;
  void *arg;
  void *result;
  void (*action)(void *);
  void (*continuation)(void *);
};

struct task_t *create_task(void (*action)(void *), void (*continuation)(void *),
                           void *arg);
struct task_t *create_task_run(void (*action)(void *),
                               void (*continuation)(void *), void *arg);
void destroy_task(struct task_t *task);

#endif