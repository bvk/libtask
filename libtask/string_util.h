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

#ifndef _LIBTASK_UTIL_H_
#define _LIBTASK_UTIL_H_

#include "libtask/base.h"

static inline bool
str2bool(const char *arg, bool *value)
{
  if (strcmp(arg, "1") == 0 || strcmp(arg, "true") == 0) {
    *value = true;
    return true;
  }

  if (strcmp(arg, "0") == 0 || strcmp(arg, "false") == 0) {
    *value = false;
    return true;
  }

  return false;
}

static inline bool
str2int32(const char *arg, int base, int32_t *valuep)
{
  char *endptr = NULL;
  long int value = strtol(arg, &endptr, base);
  if (((value == LONG_MIN || value == LONG_MAX) && errno == ERANGE) ||
      (value < INT32_MIN || value > INT32_MAX) ||
      (endptr[0] != '\0')) {
    return false;
  }
  *valuep = (int32_t) value;
  return true;
}

static inline bool
str2uint32(const char *arg, int base, int32_t *valuep)
{
  int32_t value;
  if (str2int32(arg, base, &value) && value >= 0) {
    *valuep = value;
    return true;
  }
  return false;
}

static inline bool
str2pint32(const char *arg, int base, int32_t *valuep)
{
  int32_t value;
  if (str2int32(arg, base, &value) && value > 0) {
    *valuep = value;
    return true;
  }
  return false;
}

#endif
