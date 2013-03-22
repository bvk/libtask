AM_CPPFLAGS = -I$(top_srcdir) -D_GNU_SOURCE
AM_CFLAGS = -std=gnu99 -Wall -Werror -pthread -rdynamic -fno-omit-frame-pointer
AM_CFLAGS += -Wno-unused-value -Wno-free-nonheap-object -Wno-format
AM_CFLAGS += -Wno-int-to-pointer-cast

lib_LIBRARIES = libtask.a
libtask_a_SOURCES  = task.c
libtask_a_SOURCES += task_pool.c
libtask_a_SOURCES += util/semaphore.c
libtask_a_SOURCES += util/condition.c

#
# Tests
#

TESTS = task_execute_test
bin_PROGRAMS = task_execute_test
task_execute_test_SOURCES = task_execute_test.c
task_execute_test_LDADD = libtask.a

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
