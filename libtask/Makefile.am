AM_CPPFLAGS = -I$(top_srcdir) -D_GNU_SOURCE
AM_CFLAGS = -std=gnu99 -Wall -Werror -pthread -rdynamic -fno-omit-frame-pointer
AM_CFLAGS += -Wno-unused-value -Wno-free-nonheap-object -Wno-format

lib_LIBRARIES = libtask.a
libtask_a_SOURCES  = task.c
libtask_a_SOURCES += task_pool.c
libtask_a_SOURCES += util/semaphore.c
libtask_a_SOURCES += wait_queue.c

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

TESTS += thread_local_test
bin_PROGRAMS += thread_local_test
thread_local_test_SOURCES = thread_local_test.c
thread_local_test_LDADD = libtask.a
thread_local_test_CFLAGS = $(AM_CFLAGS)
thread_local_test_CFLAGS += -Wno-int-to-pointer-cast

TESTS += producer_consumer_test
bin_PROGRAMS += producer_consumer_test
producer_consumer_test_SOURCES = producer_consumer_test.c
producer_consumer_test_LDADD = libtask.a

TESTS += producer_consumer_semaphore_test
bin_PROGRAMS += producer_consumer_semaphore_test
producer_consumer_semaphore_test_SOURCES = producer_consumer_semaphore_test.c
producer_consumer_semaphore_test_LDADD = libtask.a
