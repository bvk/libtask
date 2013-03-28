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

#ifndef _LIBTASK_UTIL_LIST_H_
#define _LIBTASK_UTIL_LIST_H_

#include "libtask/log.h"

// NOTE: These are similar to lists in Linux kernel, but are not the same!

typedef struct libtask_list {
  struct libtask_list *next;
  struct libtask_list *prev;
} libtask_list_t;

#define libtask_list_assert(link)     \
  do {				      \
    assert(link->prev->next == link); \
    assert(link->next->prev == link); \
  } while (0)

// Convert a list element back to its container type.
//
// list: The list element.
//
// type: Type of the container.
//
// member: Member name of the list element in the container.
#define libtask_list_entry(list,type,member)				\
  ((list) ? ((type*)((char*)(list) - __builtin_offsetof(type,member))) : NULL)

// Initialize a list object.
static inline void
libtask_list_initialize(libtask_list_t *list)
{
  list->next = list;
  list->prev = list;
  libtask_list_assert(list);
}

// Check if list is empty and return true or false.
static inline bool
libtask_list_empty(libtask_list_t *list)
{
  return (list->next == list);
}

// Prepend an element into a list.
//
// list: List to prepend into.
//
// link: The element to prepend.
static inline void
libtask_list_push_front(libtask_list_t *list, libtask_list_t *link)
{
  libtask_list_assert(list);
  libtask_list_assert(link);

  link->next = list->next;
  link->prev = list;
  list->next = link;
  link->next->prev = link;

  libtask_list_assert(list);
  libtask_list_assert(link);
}

// Append an element into a list.
//
// list: List to append into.
//
// link: The element to append.
static inline void
libtask_list_push_back(libtask_list_t *list, libtask_list_t *link)
{
  libtask_list_assert(list);
  libtask_list_assert(link);

  link->next = list;
  link->prev = list->prev;
  link->prev->next = link;
  link->next->prev = link;

  libtask_list_assert(list);
  libtask_list_assert(link);
}

// Remove an element from a list.
//
// link: The element to remove from a list.
static inline void
libtask_list_erase(libtask_list_t *link)
{
  libtask_list_assert(link);

  link->prev->next = link->next;
  link->next->prev = link->prev;
  link->next = link;
  link->prev = link;

  libtask_list_assert(link);
}

// Returns the first link.
static inline libtask_list_t *
libtask_list_front(libtask_list_t *list)
{
  return list->next == list ? NULL : list->next;
}

// Returns the last link.
static inline libtask_list_t *
libtask_list_back(libtask_list_t *list)
{
  return list->prev == list ? NULL : list->prev;
}

// Removes the first entry from the list.
static inline libtask_list_t *
libtask_list_pop_front(libtask_list_t *list)
{
  libtask_list_assert(list);

  if (libtask_list_empty(list)) {
    return NULL;
  }

  libtask_list_t *entry = libtask_list_front(list);
  libtask_list_erase(entry);
  return entry;
}

// Removes the last entry from the list.
static inline libtask_list_t *
libtask_list_pop_back(libtask_list_t *list)
{
  libtask_list_assert(list);

  if (libtask_list_empty(list)) {
    return NULL;
  }

  libtask_list_t *entry = libtask_list_back(list);
  libtask_list_erase(entry);
  return entry;
}

// Move a list into another list and empty the source.
static inline void
libtask_list_move(libtask_list_t *list, libtask_list_t *temp)
{
  libtask_list_assert(list);
  libtask_list_assert(temp);

  libtask_list_erase(list);
  if (!libtask_list_empty(temp)) {
    list->next = temp->next;
    list->prev = temp->prev;
    list->next->prev = list;
    list->prev->next = list;
    temp->next = temp;
    temp->prev = temp;
  }

  libtask_list_assert(list);
  libtask_list_assert(temp);
}

// Swap one list with another.
static inline void
libtask_list_swap(libtask_list_t *a, libtask_list_t *b)
{
  libtask_list_assert(a);
  libtask_list_assert(b);

  libtask_list_t t = *a;
  *a = *b;
  *b = t;

  if (b->next == a) {
    b->next = b;
    b->prev = b;
  } else {
    b->next->prev = b;
    b->prev->next = b;
  }

  if (a->next == b) {
    a->next = a;
    a->prev = a;
  } else {
    a->next->prev = a;
    a->prev->next = a;
  }

  libtask_list_assert(a);
  libtask_list_assert(b);
}

// Iterate over all list elements.
static inline void
libtask_list_apply(libtask_list_t *head, void (*funp)(int, libtask_list_t *))
{
  int index = 0;
  libtask_list_t *it = head->next;
  while (it != head) {
    libtask_list_t *next = it->next;
    funp(index++, it);
    it = next;
  }
}

//
// Helper functions.
//

// Print a list contents.
static inline void
libtask_list_print(libtask_list_t *head)
{
  libtask_list_t *it = head;
  do {
    assert(it->prev->next == it);
    fprintf(stderr, "%p->%p\n", it, it->next);
  } while ((it = it->next) != head);
}

#endif // _LIBTASK_UTIL_LIST_H_
