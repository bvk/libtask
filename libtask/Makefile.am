AM_CPPFLAGS = -I$(top_srcdir) -D_GNU_SOURCE
AM_CFLAGS = -std=gnu99 -Wall -Werror -pthread -rdynamic -fno-omit-frame-pointer
AM_CFLAGS += -Wno-unused-value

lib_LIBRARIES = libtask.a
libtask_a_SOURCES  = task.c
libtask_a_SOURCES += task_pool.c

#
# Tests
#

TESTS = task_execute_test
bin_PROGRAMS = task_execute_test
task_execute_test_SOURCES = task_execute_test.c
task_execute_test_LDADD = libtask.a

TESTS += task_schedule_test
bin_PROGRAMS += task_schedule_test
task_schedule_test_SOURCES = task_schedule_test.c
task_schedule_test_LDADD = libtask.a

TESTS += task_pool_switch_test
bin_PROGRAMS += task_pool_switch_test
task_pool_switch_test_SOURCES = task_pool_switch_test.c
task_pool_switch_test_LDADD = libtask.a
