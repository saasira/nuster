/*
 * include/nuster/nosql.h
 * This file defines everything related to nuster nosql.
 *
 * Copyright (C) [Jiang Wenyuan](https://github.com/jiangwenyuan), < koubunen AT gmail DOT com >
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, version 2.1
 * exclusively.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _NUSTER_NOSQL_H
#define _NUSTER_NOSQL_H

#include <nuster/common.h>

#define NST_NOSQL_DEFAULT_CHUNK_SIZE   32
#define NST_NOSQL_DEFAULT_LOAD_FACTOR         0.75
#define NST_NOSQL_DEFAULT_GROWTH_FACTOR       2


struct nst_nosql_element {
    struct nst_nosql_element *next;
    struct nuster_str         msg;
};

/*
 * A nst_nosql_data contains a complete http response data,
 * and is pointed by nst_nosql_entry->data.
 * All nst_nosql_data are stored in a circular singly linked list
 */
struct nst_nosql_data {
    int                       clients;
    int                       invalid;
    struct nst_nosql_element *element;
    struct nst_nosql_data    *next;
};

/*
 * A nst_nosql_entry is an entry in nst_nosql_dict hash table
 */
enum {
    NST_NOSQL_ENTRY_STATE_CREATING = 0,
    NST_NOSQL_ENTRY_STATE_VALID    = 1,
    NST_NOSQL_ENTRY_STATE_INVALID  = 2,
    NST_NOSQL_ENTRY_STATE_EXPIRED  = 3,
};

struct nst_nosql_entry {
    int                     state;
    char                   *key;
    uint64_t                hash;
    struct nst_nosql_data  *data;
    uint64_t                expire;
    uint64_t                atime;
    struct nuster_str       host;
    struct nuster_str       path;
    struct nst_nosql_entry *next;
    struct nuster_rule     *rule;        /* rule */
    int                     pid;         /* proxy uuid */
};

struct nst_nosql_dict {
    struct nst_nosql_entry **entry;
    uint64_t                 size;      /* number of entries */
    uint64_t                 used;      /* number of used entries */
#if defined NUSTER_USE_PTHREAD || defined USE_PTHREAD_PSHARED
    pthread_mutex_t          mutex;
#else
    unsigned int             waiters;
#endif
};

enum {
    NST_NOSQL_CTX_STATE_INIT   = 0,   /* init */
    NST_NOSQL_CTX_STATE_CREATE = 1,   /* to cache */
    NST_NOSQL_CTX_STATE_DONE   = 2,   /* cache done */
    NST_NOSQL_CTX_STATE_BYPASS = 3,   /* not cached, return to regular process */
    NST_NOSQL_CTX_STATE_WAIT   = 4,   /* caching, wait */
    NST_NOSQL_CTX_STATE_HIT    = 5,   /* cached, use cache */
    NST_NOSQL_CTX_STATE_PASS   = 6,   /* cache rule passed */
    NST_NOSQL_CTX_STATE_FULL   = 7,   /* cache full */
};

struct nst_nosql_ctx {
    int                       state;

    struct nuster_rule       *rule;
    struct nuster_rule_stash *stash;

    struct nst_nosql_entry   *entry;
    struct nst_nosql_data    *data;
    struct nst_nosql_element *element;

    struct {
        int                   scheme;
        struct nuster_str     host;
        struct nuster_str     uri;
        struct nuster_str     path;
        int                   delimiter;
        struct nuster_str     query;
        struct nuster_str     cookie;
    } req;
    int                       pid;         /* proxy uuid */
};

struct nst_nosql_stats {
    uint64_t        used_mem;

    struct {
        uint64_t    total;
        uint64_t    fetch;
        uint64_t    hit;
        uint64_t    abort;
    } request;
#if defined NUSTER_USE_PTHREAD || defined USE_PTHREAD_PSHARED
    pthread_mutex_t mutex;
#else
    unsigned int    waiters;
#endif
};
struct nst_nosql {
    struct nst_nosql_dict  dict[2];           /* 0: using, 1: rehashing */
    struct nst_nosql_data *data_head;         /* point to the circular linked list, tail->next ===  head */
    struct nst_nosql_data *data_tail;         /* and will be moved together constantly to check invalid data */
#if defined NUSTER_USE_PTHREAD || defined USE_PTHREAD_PSHARED
    pthread_mutex_t        mutex;
#else
    unsigned int           waiters;
#endif

    int                    rehash_idx;        /* >=0: rehashing, index, -1: not rehashing */
    int                    cleanup_idx;       /* cache dict cleanup index */
};

extern struct flt_ops  nst_nosql_filter_ops;

/* engine */
int nst_nosql_check_applet(struct stream *s, struct channel *req, struct proxy *px);
struct nst_nosql_data *nst_nosql_data_new();

/* dict */
int nst_nosql_dict_init();
struct nst_nosql_entry *nst_nosql_dict_get(const char *key, uint64_t hash);
struct nst_nosql_entry *nst_nosql_dict_set(const char *key, uint64_t hash, struct nst_nosql_ctx *ctx);
void nst_nosql_dict_rehash();
void nst_nosql_dict_cleanup();

#endif /* _NUSTER_NOSQL_H */