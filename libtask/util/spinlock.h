//
// Libtask: A thread-safe coroutine library.
//
// Copyright (C) 2013  BVK Chaitanya
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef _LIBTASK_UTIL_SPINLOCK_H_
#define _LIBTASK_UTIL_SPINLOCK_H_

#include <assert.h>
#include <stdint.h>

#include "libtask/util/atomic.h"

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

static inline bool
libtask_spinlock_status(libtask_spinlock_t *lock) {
  return libtask_atomic_load(&lock->value) == 1;
}

#endif // _LIBTASK_UTIL_SPINLOCK_H_
