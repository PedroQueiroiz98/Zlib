#include "spin_lock.h"

void spinlock_init(spinlock_t *s) { atomic_flag_clear(&s->lock); }

void spinlock_acquire(spinlock_t *s) {
  while (atomic_flag_test_and_set(&s->lock)) {
    // Busy wait (spin)
  }
}

void spinlock_release(spinlock_t *s) { atomic_flag_clear(&s->lock); }