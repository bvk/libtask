#ifndef _LIBTASK_UTIL_SPINLOCK_H_
#define _LIBTASK_UTIL_SPINLOCK_H_

#include <assert.h>
#include <stdint.h>

#include "libtask/atomic.h"

typedef struct {
  volatile int32_t value;
} libtask_spinlock_t;

static inline void
libtask_spinlock_initialize(libtask_spinlock_t *lock) {
  libtask_atomic_store(&lock->value, 1);
}

static inline void
libtask_spinlock_finalize(libtask_spinlock_t *lock) {
  assert(libtask_atomic_load(&lock->value) == 1);
}

static inline void
libtask_spinlock_lock(libtask_spinlock_t *lock) {
  while (libtask_atomic_cmpxchg(&lock->value, 1, 0) != 1) {
    continue;
  }
}

static inline void
libtask_spinlock_unlock(libtask_spinlock_t *lock) {
  libtask_atomic_store(&lock->value, 1);
}

#endif // _LIBTASK_UTIL_SPINLOCK_H_
