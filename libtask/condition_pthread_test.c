#include <pthread.h>

#include "libtask/libtask.h"

#define CHECK(x) do { if (!(x)) { assert(0); } } while (0)
#define DEBUG(fmt,...) /* printf */ (fmt, ##__VA_ARGS__)

#define NTHREADS 10

libtask_spinlock_t spinlock;
libtask_condition_t condition;

int32_t nwaiting_for_signal;
int32_t nwaiting_for_broadcast;

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
  libtask_spinlock_initialize(&spinlock);
  libtask_condition_initialize(&condition, &spinlock);

  pthread_t pthreads[NTHREADS];
  for (int i = 0; i < NTHREADS; i++) {
    CHECK(pthread_create(pthreads + i, NULL, tmain, NULL) == 0);
  }

  // Block for all threads to wait!
  DEBUG("Waiting for all threads to block\n");
  while (libtask_atomic_load(&nwaiting_for_broadcast) < NTHREADS) {
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
  while (libtask_atomic_load(&nwaiting_for_signal) < NTHREADS) {
    pthread_yield();
  }

  for (int i = 0; i < NTHREADS; i++) {
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

  for (int i = 0; i < NTHREADS; i++) {
    CHECK(pthread_join(pthreads[i], NULL) == 0);
  }

  libtask_condition_finalize(&condition);
  libtask_spinlock_finalize(&spinlock);
  return 0;
}
