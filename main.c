#include "task.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void task_run(void *arg) { printf("buscar dados no banco\n"); }
void task_run_cont(void *arg) { printf("processar dados depois da busca\n"); }

int main(void) {

  struct task_t *io = create_task_run(task_run, NULL, NULL);

  sleep(100);
  return EXIT_SUCCESS;
}