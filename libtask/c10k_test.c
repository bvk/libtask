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

//
// Testcase that simulates the c10k challenge.
//
// 1. We use two task-pools one for io and another for CPU to create
//    10k clients.
//
// 2. We use one dedicated thread (part of cpu task-pool) to use epoll
//    to accept the 10k clients and create one task for each
//    client. These tasks are executed by the CPU task-pool relying on
//    the epoll for non-blocking send/receive operations.
//
// 3. The only blocking system call "connect" here is performed by the
//    io threads and rest is CPU insentive (with the exception of
//    epoll_wait) and is handled by cpu threads of the cpu task-pool.
//
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "libtask/libtask.h"

#define CHECK(x) do { if (!(x)) { perror(""); assert(0); } } while (0)
#define DEBUG(fmt,...) /* printf */ (fmt, ##__VA_ARGS__)

#define NUM_IO_THREADS 10
#define NUM_CPU_THREADS 5

#define NSESSIONS 100 // 10000
#define NMESSAGES 100
#define TASK_STACK_SIZE (64*1024)
#define SOCKET_BACKLOG 1000

static libtask_task_pool_t *io_pool;
static libtask_task_pool_t *cpu_pool;

#define INPOOL(pool,expr)						\
  ({									\
    libtask_task_pool_t *first = libtask_get_task_pool_current();	\
    libtask_task_pool_schedule(pool);					\
    __typeof ((expr)) result = (expr);					\
    libtask_task_pool_schedule(first);					\
    result;								\
  })

#define ASYNC(expr) INPOOL(io_pool, (expr))

static int epfd = -1; // The epoll fd.
static uint16_t port_number = 0;

static uint32_t nsent = 0;
static uint32_t nreceived = 0;

static int nserved = 0;
static int nrequested = 0;

static inline void
set_nonblocking(int fd)
{
  int flags = fcntl(fd, F_GETFL);
  CHECK(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0);
}

int
client_worker_main(void *arg_)
{
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    DEBUG("%s\n", strerror(errno));
    exit(1);
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = port_number;
  CHECK(inet_aton("127.0.0.1", &addr.sin_addr) != 0);

  int r = ASYNC(connect(sockfd, (const struct sockaddr *)&addr, sizeof(addr)));
  CHECK(r == 0);
  DEBUG("connected\n");

  libtask_task_t *current = libtask_get_task_current();

  libtask_semaphore_t sem;
  libtask_semaphore_initialize(&sem, 0);

  struct epoll_event event;
  event.events = 0;
  event.data.ptr = NULL;
  r = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
  CHECK(r != -1);

  void *who = NULL;
  char buffer[128];
  for (int ii = 0; ii < NMESSAGES; ii++) {
    // Wait for a message.
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    event.data.ptr = &sem;
    r = epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &event);
    CHECK(r != -1);
    libtask_semaphore_down(&sem);

    // Receive a message.
    ssize_t nrecv = recv(sockfd, buffer, sizeof(buffer), 0);
    CHECK(nrecv >= 0);
    int jj = -1;
    CHECK(sscanf(buffer, "%p %d\n", &who, &jj) == 2);
    CHECK(ii == jj);
    libtask_atomic_add(&nreceived, 1);

    // Wait for a send.
    event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
    event.data.ptr = &sem;
    r = epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &event);
    CHECK(r != -1);
    libtask_semaphore_down(&sem);

    // Send a message.
    int size = snprintf(buffer, sizeof(buffer), "%p %d\n", current, ii) + 1;
    ssize_t cnt = send(sockfd, buffer, size, 0);
    CHECK(cnt >= 0);
    libtask_atomic_add(&nsent, 1);
  }

  r = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
  CHECK(r != -1);
  close(sockfd);

  libtask_semaphore_finalize(&sem);

  int32_t nfinished = libtask_atomic_add(&nrequested, 1);
  if (nfinished == NSESSIONS) {
    DEBUG("all clients finished\n");
  } else {
    DEBUG("clients finished: %d\n", nfinished);
  }
  return 0;
}

int
server_worker_main(void *arg_)
{
  int sockfd = (int)(uintptr_t)arg_;
  libtask_task_t *current = libtask_get_task_current();

  libtask_semaphore_t sem;
  libtask_semaphore_initialize(&sem, 0);

  struct epoll_event event;
  event.events = 0;
  event.data.ptr = NULL;
  int r = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
  CHECK(r != -1);

  void *who = NULL;
  char buffer[128];
  for (int ii = 0; ii < NMESSAGES; ii++) {
    // Wait for a send.
    event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
    event.data.ptr = &sem;
    r = epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &event);
    CHECK(r != -1);
    libtask_semaphore_down(&sem);

    // Send a message.
    int size = snprintf(buffer, sizeof(buffer), "%p %d\n", current, ii) + 1;
    ssize_t cnt = send(sockfd, buffer, size, 0);
    CHECK(cnt >= 0);
    libtask_atomic_add(&nsent, 1);

    // Wait for a reply.
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    event.data.ptr = &sem;
    r = epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &event);
    CHECK(r != -1);
    libtask_semaphore_down(&sem);

    // Receive a message.
    ssize_t nrecv = recv(sockfd, buffer, sizeof(buffer), 0);
    CHECK(nrecv >= 0);
    int jj = -1;
    CHECK(sscanf(buffer, "%p %d\n", &who, &jj) == 2);
    CHECK(ii == jj);
    libtask_atomic_add(&nreceived, 1);
  }

  r = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
  CHECK(r != -1);
  close(sockfd);

  libtask_semaphore_finalize(&sem);

  int32_t nfinished = libtask_atomic_add(&nserved, 1);
  if (nfinished == NSESSIONS) {
    DEBUG("all servers finished\n");
  } else {
    DEBUG("servers finished: %d\n", nfinished);
  }
  return 0;
}

int
listener_task_main(void *arg_)
{
  int sockfd = (int)(uintptr_t)arg_;
  set_nonblocking(sockfd);

  struct epoll_event event;
  event.events = 0;
  event.data.ptr = NULL;
  int r = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

  while (libtask_atomic_load(&nserved) < NSESSIONS ||
	 libtask_atomic_load(&nrequested) < NSESSIONS) {

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT | EPOLLERR | EPOLLPRI | EPOLLRDHUP;
    event.data.fd = sockfd;
    CHECK(epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &event) == 0);

    int r = epoll_wait(epfd, &event, 1, 10);
    if (r == -1) {
      if (errno != EINTR) {
	perror("epoll_wait");
      }
      continue;
    }

    if (r == 1) {
      if (event.data.fd == sockfd) {
	int clientfd = accept4(sockfd, NULL, NULL, SOCK_NONBLOCK);
	CHECK(clientfd >= 0);

	libtask_task_t *task = NULL;
	CHECK(libtask_task_create(&task, cpu_pool,
				  server_worker_main, (void*)clientfd,
				  TASK_STACK_SIZE) == 0);
	CHECK(task != NULL);
	DEBUG("created new task\n");
	CHECK(libtask_task_unref(task) != 0);

      } else if ((event.events & EPOLLIN) || (event.events & EPOLLOUT)) {
	/* Event is on a client fd, so schedule the corresponding task. */
	libtask_semaphore_t *sem = (libtask_semaphore_t *) event.data.ptr;
	libtask_semaphore_up(sem);

      } else {
	DEBUG("unknown event for %p\n", event.data.ptr);
      }
    }
  }
  DEBUG("listener task finished\n");

  r = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
  CHECK(r != -1);
  return 0;
}

static error_t
create_listening_socket(int backlog, int *fdp, uint16_t *portp)
{
  int fd = -1;
  while (true) {
    if (fd < 0) {
      fd = socket(AF_INET, SOCK_STREAM, 0);
      if (fd < 0) {
	return errno;
      }
    }

    uint16_t port = random() % 65536;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    CHECK(inet_aton("127.0.0.1", &addr.sin_addr) != 0);

    int r = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (r == 0) {
      if (listen(fd, backlog) == 0) {
	*fdp = fd;
	*portp = port;
	return 0;
      }
      close(fd);
      fd = -1;
      continue;
    }
    if (r < 0 && errno == EADDRINUSE) {
      port = random() % 65536;
      continue;
    }
    error_t error = errno;
    close(fd);
    return error;
  }
}

int
main(int argc, char *argv[])
{
  epfd = epoll_create1(0);
  CHECK(epfd >= 0);

  int sockfd = -1;
  CHECK(create_listening_socket(SOCKET_BACKLOG, &sockfd, &port_number) == 0);

  CHECK(libtask_task_pool_create(&io_pool) == 0);
  CHECK(libtask_task_pool_create(&cpu_pool) == 0);

  // Create listener and client tasks.
  libtask_task_t listener_task;
  CHECK(libtask_task_initialize(&listener_task,
				cpu_pool,
				listener_task_main,
				(void *)sockfd,
				TASK_STACK_SIZE) == 0);

  libtask_task_t *client_tasks = malloc(sizeof(libtask_task_t) * NSESSIONS);
  for (int i = 0; i < NSESSIONS; i++) {
    CHECK(libtask_task_initialize(&client_tasks[i],
				  cpu_pool,
				  client_worker_main,
				  NULL,
				  TASK_STACK_SIZE) == 0);
  }

  pthread_t io_threads[NUM_IO_THREADS];
  for (int i = 0; i < NUM_IO_THREADS; i++) {
    CHECK(libtask_task_pool_start(io_pool, &io_threads[i]) == 0);
  }
  pthread_t cpu_threads[NUM_CPU_THREADS];
  for (int i = 0; i < NUM_CPU_THREADS; i++) {
    CHECK(libtask_task_pool_start(cpu_pool, &cpu_threads[i]) == 0);
  }

  // Wait for tasks to finish!
  CHECK(libtask_task_wait(&listener_task) == 0);
  for (int i = 0; i < NSESSIONS; i++) {
    CHECK(libtask_task_wait(&client_tasks[i]) == 0);
  }

  // Wait for threads to finish.
  for (int i = 0; i < NUM_IO_THREADS; i++) {
    CHECK(libtask_task_pool_stop(io_pool, io_threads[i]) == 0);
    CHECK(pthread_join(io_threads[i], NULL) == 0);
  }
  for (int i = 0; i < NUM_CPU_THREADS; i++) {
    CHECK(libtask_task_pool_stop(cpu_pool, cpu_threads[i]) == 0);
    CHECK(pthread_join(cpu_threads[i], NULL) == 0);
  }

  // Destroy the tasks.
  CHECK(libtask_task_unref(&listener_task) == 0);
  for (int i = 0; i < NSESSIONS; i++) {
    CHECK(libtask_task_unref(&client_tasks[i]) == 0);
  }
  free(client_tasks);

  // Destroy the task pools.
  CHECK(libtask_task_pool_unref(io_pool) == 0);
  CHECK(libtask_task_pool_unref(cpu_pool) == 0);

  DEBUG("nsent: %d nreceived: %d\n", nsent, nreceived);
  CHECK(nsent == nreceived);
  CHECK(nsent == NMESSAGES * NSESSIONS * 2);

  close(sockfd);
  close(epfd);
  return 0;
}
