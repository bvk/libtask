#ifndef _LIBTASK_UTIL_TEST_H_
#define _LIBTASK_UTIL_TEST_H_

#include "libtask/util/log.h"

static inline bool
strtoint32(const char *arg, int base, int32_t *valuep)
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

#endif // _LIBTASK_UTIL_TEST_H_
