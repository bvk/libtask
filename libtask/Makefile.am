#
# Libtask: A thread-safe coroutine library.
#
# Copyright (C) 2013  BVK Chaitanya
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

AM_CPPFLAGS = -I$(top_srcdir) -D_GNU_SOURCE
AM_CFLAGS = -std=gnu99 -Wall -Werror -pthread -rdynamic -fno-omit-frame-pointer
AM_CFLAGS += -Wno-unused-value -Wno-free-nonheap-object -Wno-format
AM_CFLAGS += -Wno-int-to-pointer-cast

lib_LIBRARIES = libtask.a
libtask_a_SOURCES  = task.c
libtask_a_SOURCES += task_pool.c
libtask_a_SOURCES += semaphore.c
libtask_a_SOURCES += condition.c

#
# Tests
#

TESTS = basic_test
bin_PROGRAMS = basic_test
basic_test_SOURCES = basic_test.c
basic_test_LDADD = libtask.a

TESTS += producer_consumer_semaphore_test
bin_PROGRAMS += producer_consumer_semaphore_test
producer_consumer_semaphore_test_SOURCES = producer_consumer_semaphore_test.c
producer_consumer_semaphore_test_LDADD = libtask.a

TESTS += producer_consumer_condition_test
bin_PROGRAMS += producer_consumer_condition_test
producer_consumer_condition_test_SOURCES = producer_consumer_condition_test.c
producer_consumer_condition_test_LDADD = libtask.a

TESTS += c10k_test
bin_PROGRAMS += c10k_test
c10k_test_SOURCES = c10k_test.c
c10k_test_LDADD = libtask.a
c10k_test_CFLAGS = $(AM_CFLAGS) -Wno-unused-variable

TESTS += condition_pthread_test
bin_PROGRAMS += condition_pthread_test
condition_pthread_test_SOURCES = condition_pthread_test.c
condition_pthread_test_LDADD = libtask.a
