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

#include "libtask/options.h"
#include "libtask/string_util.h"

bool libtask_option_debug = false;

static struct argp_option options[] = {
  {"libtask-debug", 0, "BOOL", 0, "Print debug messages."},
  {0}
};

static error_t
libtask_options_parse(int key, char *arg, struct argp_state *state)
{
  switch (key) {
  case 0: // libtask-debug
    if (!str2bool(arg, &libtask_option_debug)) {
      argp_error(state, "invalid value %s for --%s\n", arg, options[key].name);
    }
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

// Expose libtask parser to the client.
struct argp libtask_argp = { options, libtask_options_parse };
struct argp_child libtask_argp_child = { &libtask_argp, 0, "libtask", 0 };
