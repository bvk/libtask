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

#ifndef _LIBTASK_LOG_H_
#define _LIBTASK_LOG_H_

#include "libtask/base.h"
#include "libtask/string_util.h"

#ifndef CHECK
#define CHECK(x) do { if (!(x)) { perror(""); assert(0); } } while (0)
#endif

#ifndef DEBUG
#define DEBUG(fmt,...)				\
  ({						\
    extern bool libtask_option_debug;		\
    if (libtask_option_debug) {			\
      printf (fmt, ##__VA_ARGS__);		\
    } else {					\
      (fmt, ##__VA_ARGS__);			\
    }						\
    0;						\
  })
#endif

#endif // _LIBTASK_LOG_H_
