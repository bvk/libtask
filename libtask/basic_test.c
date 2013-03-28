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

#include "libtask/libtask.h"
#include "libtask/log.h"

#define NTHREADS 10
#define NYIELD 10000
#define TASK_STACK_SIZE (16 * 1024)

static int32_t num_task_pools = 2;
static int32_t num_tasks = 1000;
static int32_t num_threads = 2;
static int32_t num_yields = 100;
static int32_t num_switches = 100;

static struct argp_option options[] = {
  {"num-task-pools", 0, "PINT32", 0, "No. of task-pools to create."},
  {"num-tasks",      1, "PINT32", 0, "No. of tasks to per task-pool."},
  {"num-threads",    2, "PINT32", 0, "No. of threads with each task-pool."},
  {"num-yields",     3, "UINT32", 0, "No. of yields to perform by the task."},
  {"num-switches",   4, "UINT32", 0, "No. of task-pool switches to perform."},
  {0}
};

int
work(void *arg_)
{
  libtask_task_pool_t *task_pools = (libtask_task_pool_t *)arg_;

  int nyields = 0;
  int nswitches = 0;

  while (nyields < num_yields || nswitches < num_switches) {
    switch (random() % 2) {
    case 0: // yield
      nyields++;
      libtask_yield();
      break;

    case 1: // switch
      nswitches++;
      libtask_task_pool_schedule(&task_pools[random() % num_task_pools]);
      break;
    }
  }
  return 0;
}

static error_t
parse_options(int key, char *arg, struct argp_state *state)
{
  switch (key) {
  case 0: // num-task-pools
    if (!str2pint32(arg, 10, &num_task_pools)) {
      argp_error(state, "Invalid value %s for --%s\n", arg, options[key].name);
    }
    break;

  case 1: // num-tasks
    if (!str2pint32(arg, 10, &num_tasks)) {
      argp_error(state, "Invalid value %s for --%s\n", arg, options[key].name);
    }
    break;

  case 2: // num-threads
    if (!str2pint32(arg, 10, &num_threads)) {
      argp_error(state, "Invalid value %s for --%s\n", arg, options[key].name);
    }
    break;

  case 3: // num-yields
    if (!str2uint32(arg, 10, &num_yields)) {
      argp_error(state, "Invalid value %s for --%s\n", arg, options[key].name);
    }
    break;

  case 4: // num-switches
    if (!str2uint32(arg, 10, &num_switches)) {
      argp_error(state, "Invalid value %s for --%s\n", arg, options[key].name);
    }
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

int
main(int argc, char *argv[])
{
  struct argp_child children[2];
  children[0] = libtask_argp_child;
  children[1] = (struct argp_child){0};

  struct argp argp = { options, parse_options, 0, 0, children };
  argp_parse(&argp, argc, argv, 0, 0, 0);

  // Create task pools.
  libtask_task_pool_t *task_pools =
    malloc(sizeof (libtask_task_pool_t) * num_task_pools);
  CHECK(task_pools);
  for (int i = 0; i < num_task_pools; i++) {
    CHECK(libtask_task_pool_initialize(&task_pools[i]) == 0);
  }

  // Create tasks in each task pool.
  libtask_task_t *tasks =
    malloc(sizeof (libtask_task_t) * num_task_pools * num_tasks);
  CHECK(tasks);
  for (int i = 0; i < num_tasks * num_task_pools; i++) {
    libtask_task_pool_t *task_pool = &task_pools[i % num_task_pools];
    CHECK(libtask_task_initialize(&tasks[i], task_pool, work, task_pools,
				  TASK_STACK_SIZE) == 0);
  }

  // Create threads for each task-pool.
  pthread_t *threads =
    malloc(sizeof (pthread_t) * num_task_pools * num_threads);
  CHECK(threads);
  for (int i = 0; i < num_threads * num_task_pools; i++) {
    libtask_task_pool_t *task_pool = &task_pools[i % num_task_pools];
    CHECK(libtask_task_pool_start(task_pool, &threads[i]) == 0);
  }

  // Wait for all tasks to finish.
  for (int i = 0; i < num_tasks * num_task_pools; i++) {
    CHECK(libtask_task_wait(&tasks[i]) == 0);
  }

  // Stop and kill all threads.
  for (int i = 0; i < num_threads * num_task_pools; i++) {
    libtask_task_pool_t *task_pool = &task_pools[i % num_task_pools];
    CHECK(libtask_task_pool_stop(task_pool, threads[i]) == 0);
    CHECK(pthread_join(threads[i], NULL) == 0);
  }

  // Kill all tasks.
  for (int i = 0; i < num_tasks * num_task_pools; i++) {
    CHECK(libtask_task_unref(&tasks[i]) == 0);
  }

  // Kill all task-pools.
  for (int i = 0; i < num_task_pools; i++) {
    CHECK(libtask_task_pool_unref(&task_pools[i]) == 0);
  }

  free(threads);
  free(tasks);
  free(task_pools);
  return 0;
}
