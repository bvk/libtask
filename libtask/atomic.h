#ifndef _LIBTASK_ATOMIC_H_
#define _LIBTASK_ATOMIC_H_

// Macros for atomic operations.

#define libtask_atomic_load(x) __atomic_load_n((x), __ATOMIC_SEQ_CST)
#define libtask_atomic_store(x,n) __atomic_store_n((x), (n), __ATOMIC_SEQ_CST)
#define libtask_atomic_add(x,n) __atomic_add_fetch((x), (n), __ATOMIC_SEQ_CST)
#define libtask_atomic_sub(x,n) __atomic_sub_fetch((x), (n), __ATOMIC_SEQ_CST)

#define libtask_atomic_cmpxchg(p,o,n)					\
  ({									\
    __typeof ((o)) tmp = (o);						\
    __atomic_compare_exchange_n((p), &tmp, (n),				\
				true /* strong */,			\
				__ATOMIC_SEQ_CST,			\
				__ATOMIC_SEQ_CST);			\
    tmp;								\
  })

#endif // _LIBTASK_ATOMIC_H_
