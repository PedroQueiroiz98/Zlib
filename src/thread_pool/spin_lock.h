#ifndef SPIN_LOCK_H
#define SPIN_LOCK_H

#include <stdatomic.h>
typedef struct {
  atomic_flag lock;
} spinlock_t;

void spinlock_init(spinlock_t *s);
void spinlock_acquire(spinlock_t *s);
void spinlock_release(spinlock_t *s);

#endif