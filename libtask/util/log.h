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

#ifndef _LIBTASK_UTIL_LOG_H_
#define _LIBTASK_UTIL_LOG_H_

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CHECK
#define CHECK(x) do { if (!(x)) { perror(""); assert(0); } } while (0)
#endif

#ifndef DEBUG
#define DEBUG(fmt,...) do { /* printf */ (fmt, ##__VA_ARGS__); } while (0)
#endif

#endif // _LIBTASK_UTIL_LOG_H_
