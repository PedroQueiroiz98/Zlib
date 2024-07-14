
#include "spin_lock.h"
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

typedef void (*call_back)(void *);

struct work_item_t {
  struct work_item_t *next;
  call_back action;
  void *arg;
};
struct work_queue_t {
  struct work_item_t *head;
  struct work_item_t *tail;
  pthread_mutex_t mutext;
  pthread_cond_t cond_event;
  spinlock_t spin_lock;
};

#define INITIAL_STEALING_QUEUE 32

struct work_steleaing_queue_t {

  struct work_item_t **works_itens;
  atomic_int head_index;
  atomic_int tail_index;
  spinlock_t spin_lock;
  int m_mask;
  int length;
};

static struct work_queue_t *create_work_queue() {

  struct work_queue_t *queue =
      (struct work_queue_t *)malloc(sizeof(struct work_queue_t));
  assert(queue != NULL);
  queue->head = NULL;
  queue->tail = NULL;

  pthread_mutex_init(&queue->mutext, NULL);
  pthread_cond_init(&queue->cond_event, NULL);
  spinlock_init(&queue->spin_lock);

  return queue;
}
static void notify_push(struct work_queue_t *queue) {
  pthread_mutex_lock(&queue->mutext);
  pthread_cond_signal(&queue->cond_event);
  pthread_mutex_unlock(&queue->mutext);
}
static void push(struct work_queue_t *queue, call_back action, void *arg) {

  struct work_item_t *item =
      (struct work_item_t *)malloc(sizeof(struct work_item_t));

  assert(item != NULL);

  item->action = action;
  item->arg = arg;

  spinlock_acquire(&queue->spin_lock);
  if (queue->head == NULL) {

    queue->head = item;
    queue->tail = item;
    notify_push(queue);
  } else {
    queue->tail->next = item;
    queue->tail = item;
    notify_push(queue);
  }
  spinlock_release(&queue->spin_lock);
}

static struct work_item_t *pop(struct work_queue_t *queue) {

  spinlock_acquire(&queue->spin_lock);
  struct work_item_t *item = queue->head;
  if (item != NULL) {
    queue->head = item->next;
  }
  spinlock_release(&queue->spin_lock);

  return item;
}
static int queue_is_empty(const struct work_queue_t *queue) {
  return queue->head == NULL;
}
static void await_work_itens(struct work_queue_t *queue) {

  pthread_mutex_lock(&queue->mutext);
  while (queue_is_empty(queue)) {
    pthread_cond_wait(&queue->cond_event, &queue->mutext);
    pthread_mutex_unlock(&queue->mutext);
  }
}
static void destroy_work_queue(struct work_queue_t *queue) {

  pthread_mutex_destroy(&queue->mutext);
  pthread_cond_destroy(&queue->cond_event);
  free(queue);
}

static struct work_steleaing_queue_t *create_stealing_queue() {

  struct work_steleaing_queue_t *queue =
      (struct work_steleaing_queue_t *)malloc(
          sizeof(struct work_steleaing_queue_t));

  queue->head_index = 0;
  queue->tail_index = 0;
  queue->m_mask = INITIAL_STEALING_QUEUE - 1;
  queue->length = INITIAL_STEALING_QUEUE;
  queue->works_itens = (struct work_item_t **)malloc(
      sizeof(struct work_item_t *) * INITIAL_STEALING_QUEUE);
  spinlock_init(&queue->spin_lock);

  return queue;
}
static void push_local(struct work_steleaing_queue_t *queue, call_back action,
                       void *arg) {

  struct work_item_t *item =
      (struct work_item_t *)malloc(sizeof(struct work_item_t));

  assert(item != NULL);

  item->action = action;
  item->arg = arg;

  int tail = queue->tail_index;

  if (tail < queue->head_index + queue->m_mask) {
    queue->works_itens[tail] = item;
    queue->tail_index = tail + 1;
  } else {

    spinlock_acquire(&queue->spin_lock);

    int head = queue->head_index;
    int count = queue->tail_index - queue->head_index;

    if (count > queue->m_mask) {

      struct work_item_t **itens = (struct work_item_t **)malloc(
          sizeof(struct work_item_t *) * (queue->length << 1));

      for (int i = 0; i < queue->length; i++)
        itens[i] = queue->works_itens[(i + head) & queue->m_mask];

      free(queue->works_itens);
      queue->works_itens = itens;
      queue->head_index = 0;
      queue->tail_index = tail = count;
      queue->m_mask = (queue->m_mask << 1) | 1;
      queue->length = queue->length << 1;
    }

    queue->works_itens[tail] = item;
    queue->tail_index = tail + 1;

    spinlock_release(&queue->spin_lock);
  }
}

static struct work_item_t *
local_pop_core(struct work_steleaing_queue_t *queue) {

  while (1) {

    int tail = queue->tail_index;
    if (queue->tail_index >= tail) {
      return NULL;
    }

    tail--;
    atomic_exchange(&queue->tail_index, tail);

    if (queue->head_index <= tail) {

      int idex = tail & queue->m_mask;
      struct work_item_t *item = queue->works_itens[idex];

      if (item == NULL)
        continue;

      queue->works_itens[idex] = NULL;
      return item;
    } else {

      spinlock_acquire(&queue->spin_lock);

      if (queue->head_index <= tail) {

        int idex = tail & queue->m_mask;
        struct work_item_t *item = queue->works_itens[idex];

        if (item == NULL)
          continue;

        queue->works_itens[idex] = NULL;
        spinlock_release(&queue->spin_lock);
        return item;

      } else {

        queue->tail_index = tail + 1;
        spinlock_release(&queue->spin_lock);
        return NULL;
      }
    }
  }
}

static inline int can_steal(struct work_steleaing_queue_t *queue) {
  return queue->head_index < queue->tail_index;
}

static struct work_item_t *try_steal(struct work_steleaing_queue_t *queue,
                                     int *missedSteal) {

  while (1) {

    if (can_steal(queue)) {

      spinlock_acquire(&queue->spin_lock);

      int head = queue->head_index;
      atomic_exchange(&queue->head_index, head + 1);

      if (head < queue->tail_index) {

        int idex = head & queue->m_mask;
        struct work_item_t *item = queue->works_itens[idex];
        if (item == NULL)
          continue;

        queue->works_itens[idex] = NULL;
        spinlock_release(&queue->spin_lock);
        return item;

      } else {

        queue->head_index = head;
      }

      *missedSteal = 1;
    }
    spinlock_release(&queue->spin_lock);

    return NULL;
  }
}

static inline void
destroy_stealing_queue(struct work_steleaing_queue_t *queue) {

  free(queue);
}