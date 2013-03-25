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

#ifndef _LIBTASK_UTIL_SEMAPHORE_H_
#define _LIBTASK_UTIL_SEMAPHORE_H_

#include <pthread.h>

#include "libtask/task.h"
#include "libtask/task_pool.h"
#include "libtask/util/list.h"
#include "libtask/util/log.h"
#include "libtask/util/spinlock.h"

typedef struct {
  libtask_spinlock_t spinlock;
  int64_t count;
  libtask_list_t waiting_list;
} libtask_semaphore_t;

// Initialize a semaphore.
//
// sem: The semaphore.
//
// count: The initial value.
void
libtask_semaphore_initialize(libtask_semaphore_t *sem, int32_t count);

// Destroy a semaphore. No tasks must be waiting on the semaphore.
//
// sem: The semaphore.
void
libtask_semaphore_finalize(libtask_semaphore_t *sem);

// Up a semaphore.
//
// sem: The semaphore.
void
libtask_semaphore_up(libtask_semaphore_t *sem);

// Down a semaphore and wait if necessary. This function should be
// called only from task context.
//
// sem: The semaphore.
void
libtask_semaphore_down(libtask_semaphore_t *sem);

#endif // _LIBTASK_UTIL_SEMAPHORE_H_
