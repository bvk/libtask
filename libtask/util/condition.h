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

#ifndef _LIBTASK_UTIL_CONDITION_H_
#define _LIBTASK_UTIL_CONDITION_H_

#include <pthread.h>

#include "libtask/util/list.h"
#include "libtask/util/spinlock.h"

// This condition variable implementation that works with
// spinlocks. It can be used from task and normal thread contexts.
//
// NOTE: Signal, Broadcast and Wait expect the caller to hold the
// spinlock.

typedef struct {
  // The spinlock this condition variable is associated with.
  libtask_spinlock_t *spinlock;

  // List of tasks waiting on this condition variable.
  libtask_list_t list;

  // The pthread condition variable for normal threads to wait.
  pthread_cond_t cond;

  // The pthread mutex that is required by the pthread condition
  // variable.
  pthread_mutex_t mutex;
} libtask_condition_t;

// Initialize a condition variable. Condition variables are always
// associated with a spinlock, so the spinlock parameter must also be
// specified. Note that multiple condition variables can be associated
// with a same spinlock.
//
// cond: The condition variable.
//
// spinlock: The spinlock.
void
libtask_condition_initialize(libtask_condition_t *cond,
			     libtask_spinlock_t *spinlock);

// Destroy a condition variable and free up its resources.
//
// cond: The condition variable.
void
libtask_condition_finalize(libtask_condition_t *cond);

// Wait on a condition variable. Tasks or normal threads can wait for
// an event on the condition variable. The spinlock this condition
// variable is associated with must be locked by the callers.
//
// cond: The condition variable.
void
libtask_condition_wait(libtask_condition_t *cond);

// Wake up one task or a thread waiting on the condition variable.
// The spinlock this condition variable is associated with must be
// locked by the caller. When a task is woken, its task-pool is also
// signaled.
//
// cond: The condition variable.
void
libtask_condition_signal(libtask_condition_t *cond);

// Wake up all tasks and threads waiting on the condition
// variable. The spinlock this condition variable is associated with
// must be locked by the caller. For every task that is woken up, its
// task-pool is also signaled.
//
// cond: The condition variable.
void
libtask_condition_broadcast(libtask_condition_t *cond);

#endif // _LIBTASK_UTIL_CONDITION_H_
