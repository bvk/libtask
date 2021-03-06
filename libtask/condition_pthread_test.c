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

#include <argp.h>
#include <pthread.h>

#include "libtask/libtask.h"
#include "libtask/log.h"

static int32_t num_threads = 10;

libtask_spinlock_t spinlock;
libtask_condition_t condition;

int32_t nwaiting_for_signal;
int32_t nwaiting_for_broadcast;

static struct argp_option options[] = {
  {"num-threads", 0, "PINT32", 0, "No. of threads in the task-pool."},
  {0}
};

static error_t
parse_options(int key, char *arg, struct argp_state *state)
{
  switch(key) {
  case 0: // num-threads
    if (!str2pint32(arg, 10, &num_threads)) {
      argp_error(state, "Invalid value %s for --%s\n", arg, options[key].name);
    }
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

void *
tmain(void *arg_)
{
  // Wait for condition broadcast!
  libtask_spinlock_lock(&spinlock);
  DEBUG("broadcast: %d\n", libtask_atomic_add(&nwaiting_for_broadcast, 1));
  libtask_condition_wait(&condition);
  DEBUG("broadcast: %d\n", libtask_atomic_sub(&nwaiting_for_broadcast, 1));
  libtask_spinlock_unlock(&spinlock);

  // Block for all threads to resume!
  while (libtask_atomic_load(&nwaiting_for_broadcast) != 0) {
    pthread_yield();
  }

  // Wait for condition singal!
  libtask_spinlock_lock(&spinlock);
  DEBUG("signal: %d\n", libtask_atomic_add(&nwaiting_for_signal, 1));
  libtask_condition_wait(&condition);
  DEBUG("signal: %d\n", libtask_atomic_sub(&nwaiting_for_signal, 1));
  libtask_spinlock_unlock(&spinlock);

  // Block for all threads to resume!
  while (libtask_atomic_load(&nwaiting_for_signal) != 0) {
    pthread_yield();
  }

  return NULL;
}

int
main(int argc, char *argv[])
{
  struct argp_child children[2];
  children[0] = libtask_argp_child;
  children[1] = (struct argp_child){0};

  struct argp argp = { options, parse_options, 0, 0, children };
  argp_parse(&argp, argc, argv, 0, 0, 0);

  libtask_spinlock_initialize(&spinlock);
  libtask_condition_initialize(&condition, &spinlock);

  pthread_t pthreads[num_threads];
  for (int i = 0; i < num_threads; i++) {
    CHECK(pthread_create(pthreads + i, NULL, tmain, NULL) == 0);
  }

  // Block for all threads to wait!
  DEBUG("Waiting for all threads to block\n");
  while (libtask_atomic_load(&nwaiting_for_broadcast) < num_threads) {
    pthread_yield();
  }

  // Broadcast!
  libtask_spinlock_lock(&spinlock);
  libtask_condition_broadcast(&condition);
  libtask_spinlock_unlock(&spinlock);

  // Block for all threads to resume!
  DEBUG("Waiting for all threads to resume\n");
  while (libtask_atomic_load(&nwaiting_for_broadcast) > 0) {
    pthread_yield();
  }

  // Block for all threads to wait again!
  DEBUG("Waiting for all threads to block again\n");
  while (libtask_atomic_load(&nwaiting_for_signal) < num_threads) {
    pthread_yield();
  }

  for (int i = 0; i < num_threads; i++) {
    DEBUG("Wakup %d\n", i);
    libtask_spinlock_lock(&spinlock);
    libtask_condition_signal(&condition);
    libtask_spinlock_unlock(&spinlock);
    pthread_yield();
  }

  // Block for all threads to resume!
  DEBUG("Waiting for all threads to resme\n");
  while (libtask_atomic_load(&nwaiting_for_signal) > 0) {
    pthread_yield();
  }

  for (int i = 0; i < num_threads; i++) {
    CHECK(pthread_join(pthreads[i], NULL) == 0);
  }

  libtask_condition_finalize(&condition);
  libtask_spinlock_finalize(&spinlock);
  return 0;
}
