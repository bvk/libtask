#ifndef _LIBTASK_SRC_ATOMIC_H_
#define _LIBTASK_SRC_ATOMIC_H_

// Macros for atomic operations.

#define libtask_atomic_load(x) __atomic_load_n((x), __ATOMIC_SEQ_CST)
#define libtask_atomic_store(x,n) __atomic_store_n((x), (n), __ATOMIC_SEQ_CST)
#define libtask_atomic_add(x,n) __atomic_add_fetch((x), (n), __ATOMIC_SEQ_CST)
#define libtask_atomic_sub(x,n) __atomic_sub_fetch((x), (n), __ATOMIC_SEQ_CST)

#endif // _LIBTASK_SRC_ATOMIC_H_
