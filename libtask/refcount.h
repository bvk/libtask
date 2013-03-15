#ifndef _LIBTASK_REFCOUNT_H_
#define _LIBTASK_REFCOUNT_H_

#include "libtask/atomic.h"

// Reference Counting
//
// We use reference counting to decide when an object's destructor
// should be invoked.  Since memory has to be freed only for heap
// objects, we should know if the object was heap allocated or stack
// allocated.  We learn this as follows: we increment/decrement the
// refcount by two and depending on the initial value, last refcount
// has two possible values.  One of them is used for heap objects and
// the other for stack objects.

typedef struct {
  volatile int32_t count;
} libtask_refcount_t;

// Initialize refcount in a stack allocated object.
#define libtask_refcount_initialize(x)			\
  do {							\
    libtask_atomic_store(&(x)->count, 2);		\
  } while (0)

// Initialize refcount in a heap allocated object.
#define libtask_refcount_create(x)			\
  do {							\
    libtask_atomic_store(&(x)->count, 3);		\
  } while (0)

// Returns the current reference count.
#define libtask_refcount_count(x)		\
  (libtask_atomic_load(&(x)->count) / 2)

// Increment an object's reference count.
#define libtask_refcount_inc(x)			\
  do {						\
    libtask_atomic_add(&(x)->count, 2);		\
  } while (0)

// Decrement an object's reference count and run the destructor if
// necessary. Return the reference count in nrefp.
#define libtask_refcount_dec(x,dtor,obj,nrefp)		\
  do {							\
    int32_t count = libtask_atomic_sub(&(x)->count, 2);	\
    assert(count >= 0);					\
    if ((*nrefp = count / 2)) {				\
      break;						\
    }							\
    dtor((obj));					\
    if (count == 1) {					\
      free((x));					\
    }							\
  } while (0)

#endif // _LIBTASK_REFCOUNT_H_
