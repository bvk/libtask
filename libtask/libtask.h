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

#ifndef _LIBTASK_LIBTASK_H_
#define _LIBTASK_LIBTASK_H_

#include <argp.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include "libtask/task.h"
#include "libtask/task_pool.h"
#include "libtask/semaphore.h"
#include "libtask/spinlock.h"
#include "libtask/condition.h"

// Command line options for configuring the library.
extern struct argp libtask_argp;
extern struct argp_child libtask_argp_child;

// Schedule the current task and give up the thread.
static inline error_t
libtask_yield() {
  libtask_task_t *current = libtask_get_task_current();
  if (!current) {
    return EINVAL;
  }
  return libtask_task_pool_schedule(current->owner);
}

// Get the current thread's id.
static inline int32_t
libtask_thread_id()
{
  pid_t tid;
  tid = syscall(SYS_gettid);
  return tid;
}

// Get the current time in microseconds.
static inline int64_t
libtask_now_usecs()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t) (tv.tv_sec) * 1000000 + tv.tv_usec;
}

#endif // _LIBTASK_LIBTASK_H_
