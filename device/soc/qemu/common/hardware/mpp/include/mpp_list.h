/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MPP_LIST_H__
#define __MPP_LIST_H__

#include "rk_type.h"

#include "mpp_thread.h"

/*
 * two list structures are defined, one for C++, the other for C
 */

#ifdef __cplusplus

// desctructor of list node
typedef void *(*node_destructor)(void *);

struct mpp_list_node;

class mpp_list : public MppMutexCond {
public:
    mpp_list(node_destructor func = NULL);
    ~mpp_list();

    // for FIFO or FILO implement
    // adding functions support simple structure like C struct or C++ class pointer,
    // do not support C++ object
    signed int add_at_head(void *data, signed int size);
    signed int add_at_tail(void *data, signed int size);
    // deleting function will copy the stored data to input pointer with size as size
    // if NULL is passed to deleting functions, the node will be delete directly
    signed int del_at_head(void *data, signed int size);
    signed int del_at_tail(void *data, signed int size);

    // direct fifo operation
    signed int fifo_wr(void *data, signed int size);
    signed int fifo_rd(void *data, signed int *size);

    // for status check
    signed int list_is_empty();
    signed int list_size();

    // for vector implement - not implemented yet
    // adding function will return a key
    signed int add_by_key(void *data, signed int size, unsigned int *key);
    signed int del_by_key(void *data, signed int size, unsigned int key);
    signed int show_by_key(void *data, unsigned int key);

    signed int flush();

private:
    node_destructor destroy;
    struct mpp_list_node *head;
    signed int count;
    static unsigned int keys;
    static unsigned int get_key();

    mpp_list(const mpp_list &);
    mpp_list &operator=(const mpp_list &);
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name)                                                                                           \
    {                                                                                                                  \
        &(name), &(name)                                                                                               \
    }

#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr)                                                                                            \
    do {                                                                                                               \
        (ptr)->next = (ptr);                                                                                           \
        (ptr)->prev = (ptr);                                                                                           \
    } while (0)

#define list_for_each_safe(pos, n, head)                                                                               \
    do {                                                                                                               \
        for ((pos) = (head)->next, (n) = (pos)->next; (pos) != (head); (pos) = (n), (n) = (pos)->next)                 \
    } while (0)

#define list_entry(ptr, type, member) ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define list_first_entry(ptr, type, member) list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) list_entry((ptr)->prev, type, member)

#define list_first_entry_or_null(ptr, type, member)                                                                    \
    do {                                                                                                               \
        struct list_head *head__ = (ptr);                                                                              \
        struct list_head *pos__ = head__->next;                                                                        \
        pos__ != head__ ? list_entry(pos__, type, member) : NULL;                                                      \
    } while (0)

#define list_next_entry(pos, type, member) list_entry((pos)->member.next, type, member)

#define list_prev_entry(pos, type, member) list_entry((pos)->member.prev, type, member)

#define list_for_each_entry(pos, head, type, member)                                                                   \
    for ((pos) = list_entry((head)->next, type, member); &(pos)->member != (head);                                     \
         (pos) = list_next_entry((pos), type, member))

#define list_for_each_entry_safe(pos, n, head, type, member)                                                           \
    for ((pos) = list_first_entry(head, type, member), (n) = list_next_entry((pos), type, member);                     \
         &(pos)->member != (head); (pos) = (n), (n) = list_next_entry(n, type, member))

#define list_for_each_entry_reverse(pos, head, type, member)                                                           \
    for ((pos) = list_last_entry(head, type, member); &(pos)->member != (head);                                        \
         (pos) = list_prev_entry((pos), type, member))

#define list_for_each_entry_safe_reverse(pos, n, head, type, member)                                                   \
    for ((pos) = list_last_entry(head, type, member), (n) = list_prev_entry((pos), type, member);                      \
         &(pos)->member != (head); (pos) = (n), (n) = list_prev_entry(n, type, member))

static __inline void _list_add(struct list_head *_new, struct list_head *prev, struct list_head *next)
{
    next->prev = _new;
    _new->next = next;
    _new->prev = prev;
    prev->next = _new;
}

static __inline void list_add(struct list_head *_new, struct list_head *head)
{
    _list_add(_new, head, head->next);
}

static __inline void list_add_tail(struct list_head *_new, struct list_head *head)
{
    _list_add(_new, head->prev, head);
}

static __inline void _list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

static __inline void list_del_init(struct list_head *entry)
{
    _list_del(entry->prev, entry->next);

    INIT_LIST_HEAD(entry);
}

static __inline int list_is_last(const struct list_head *list, const struct list_head *head)
{
    return list->next == head;
}

static __inline int list_empty(struct list_head *head)
{
    return head->next == head;
}

#ifdef __cplusplus
}
#endif

#endif /* __MPP_LIST_H__ */
