#include <assert.h>
#include <stdlib.h>

struct fast_random_t {
  unsigned int w, x, y, z;
};

static void init_fast_random(struct fast_random_t *fast_random,
                             unsigned int seed) {
  fast_random->x = seed;
  fast_random->w = 88675123;
  fast_random->y = 362436069;
  fast_random->z = 521288629;
}

static int fast_random_next(struct fast_random_t *fast_random, int maxValue) {

  unsigned int t = fast_random->x ^ (fast_random->x << 11);
  fast_random->x = fast_random->y;
  fast_random->y = fast_random->z;
  fast_random->z = fast_random->w;
  fast_random->w = fast_random->w ^ (fast_random->w >> 19) ^ (t ^ (t >> 8));

  return (int)(fast_random->w % (unsigned int)maxValue);
}
