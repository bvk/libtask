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
