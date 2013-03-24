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

#include <assert.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "libtask/libtask.h"

#define CHECK(x) do { if (!(x)) { assert(0); } } while (0)
#define DEBUG(fmt,...) printf (fmt, ##__VA_ARGS__)

#define NTHREADS 10
#define NYIELD 10000
#define TASK_STACK_SIZE (16 * 1024)

int
work(void *arg_)
{
  volatile int *count = (volatile int *)arg_;
  for (int i = 0; i < NYIELD; i++) {
    *count += 1;

    CHECK(libtask_yield() == 0);
  }
  return 0;
}

int
main(int argc, char *argv[])
{
  libtask_task_pool_t task_pool;
  CHECK(libtask_task_pool_initialize(&task_pool) == 0);

  int counter = 0;
  libtask_task_t task;
  CHECK(libtask_task_initialize(&task, &task_pool, work, &counter,
				TASK_STACK_SIZE) == 0);

  pthread_t pthread[NTHREADS];
  for (int i = 0; i < NTHREADS; i++) {
    CHECK(libtask_task_pool_start(&task_pool, &pthread[i]) == 0);
  }
  CHECK(libtask_task_wait(&task) == 0);
  for (int i = 0; i < NTHREADS; i++) {
    CHECK(libtask_task_pool_stop(&task_pool, pthread[i]) == 0);
    CHECK(pthread_join(pthread[i], NULL) == 0);
  }

  CHECK(counter == NYIELD);

  CHECK(libtask_task_unref(&task) == 0);
  CHECK(libtask_task_pool_unref(&task_pool) == 0);
  return 0;
}
