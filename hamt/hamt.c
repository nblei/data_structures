#include "hamt.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define HAMT_VALID 0x815842
#define valid_hamt(t) ((t)->valid == HAMT_VALID)
#define HAMT_MAX_LEVEL 5
#define HAMT_CREATE 1
#define HAMT_NOCREATE 0

/* For removing values */
#define HAMT_NOREMOVE 0
#define HAMT_REMOVECLEAR 1
#define HAMT_REMOVENOCLEAR 2

#define HAMT_MASK32 0xffffffff

#define find_logical_index(hash, depth) ((hash >> ((depth) * 5)) & 0x01f)

#define HAMT_ARRAY_ADD 1
#define HAMT_ARRAY_REMOVE 2

struct hamt_list {
        void * key;
        void * value;
        struct hamt_list * next;
};

typedef struct hamt_node hamt_n;
struct hamt_node {
        struct hamt_list * values;
        uint32_t size;
        uint32_t bitfield;
        struct hamt_node * children[32];
};

typedef struct {
        struct hamtinfo info;
        hamt_n * root;
        int valid;
} hamt_s;

hamt_n * _create_hamt_node(void);
int _insert_hamt_list(hamt_s * s, hamt_n * node, const void * key, const void * val);
int _remove_hamt_list(hamt_s * s, hamt_n * node, const void * key, void ** buf);
int _find_hamt_list(hamt_s * s, hamt_n * node, const void * key, void ** buf);
struct hamt_list * _make_hamt_list_node(hamt_s * s, const void * key,
                const void * val, struct hamt_list * next);
hamt_n * __find_hamt(hamt_s * s, hamt_n * root, const int hash, const int depth);
int _find_hamt(hamt_s * s, hamt_n * root, const int hash, 
                const void * key, void ** buf);
void _free_hamt_nodes(hamt_s * s, hamt_n * root);
void _free_hamt_node(hamt_s * s, hamt_n * root);
int _remove_hamt(hamt_s * s, hamt_n * root, const int hash, const int depth,
                const void * key, void **buf);

HAMT * init_hamt(struct hamtinfo * info)
{
        if (info == NULL) {
                errno = EINVAL;
                return NULL;
        }

        hamt_s * rv = (hamt_s*)calloc(1, sizeof(*rv));
        if (rv == NULL) return NULL;
        rv->root = (hamt_n *)calloc(1, sizeof(*(rv->root)));
        if (rv->root == NULL) {
                free(rv);
                return NULL;
        }

        memcpy(&rv->info, info, sizeof(*info));
        rv->valid = HAMT_VALID;

        return (HAMT*)rv;
}

int _insert_ham(hamt_s * h, hamt_n * root, const int hash, const int depth,
                const void * key, const void * val);

int insert_hamt(HAMT * H, void * key, void * val)
{
        hamt_s * h = (hamt_s*)H;
        if (h->valid != HAMT_VALID) {
                errno = EINVAL;
                return -1;
        }

        int hash = h->info.hash(key);
        int rv = _insert_ham(h, h->root, hash, 0, key, val);
        return rv;
}

int _insert_ham(hamt_s * h, hamt_n * root, const int hash, const int depth,
                const void * key, const void * val)
{
        if (depth == HAMT_MAX_LEVEL) {
                int rv = _insert_hamt_list(h, root, key, val);
                root->size += rv;
                return rv;
        }

        int logical_index = find_logical_index(hash, depth);

        if (root->children[logical_index] == NULL)
                root->children[logical_index] = _create_hamt_node();

        int rv = _insert_ham(h, root->children[logical_index], hash, depth+1,
                        key, val);

        root->bitfield |= (1 << logical_index);
        root->size += rv;
        return rv;
}

hamt_n * _create_hamt_node(void)
{
        hamt_n * rv = (hamt_n*)calloc(1, sizeof(*rv));
        return rv;
}

struct hamt_list * _make_hamt_list_node(hamt_s * s, const void * key,
                const void * val, struct hamt_list * next)
{
        struct hamt_list * rv = (struct hamt_list *)malloc(sizeof(*rv));
        rv->key   = s->info.copy_key(key);
        rv->value = s->info.copy_elem(val);
        rv-> next = next;
        return rv;
}

int _insert_hamt_list(hamt_s * s, hamt_n * node,
                const void * key, const void * val)
{
        if (node->values == NULL) {
                node->values = _make_hamt_list_node(s, key, val, NULL);
                return 1;
        }

        struct hamt_list * cur = node->values, * prev = NULL;
        while (cur) {
                if (s->info.cmp_key(key, cur->key) == 0) {
                        s->info.free_elem(cur->value);
                        cur->value = s->info.copy_elem(val);
                        return 0;
                }

                prev = cur;
                cur = cur->next;
        }

        prev->next = _make_hamt_list_node(s, key, val, NULL);
        return 1;
}

/*
 * Fills buffer with value asociated with key and returns 1 on success
 * If key is not found, returns 0
 */
int _find_hamt_list(hamt_s * s, hamt_n * node, const void * key, void ** buf)
{
        if (node == NULL) {
                fprintf(stderr, "Node is NULL at line: %d\n", __LINE__);
        }
        struct hamt_list * it = node->values;
        while (it) {
                if (s->info.cmp_key(it->key, key) == 0) {
                        *buf = s->info.copy_elem(it->value);
                        return 1;
                }
                it = it->next;
        }
        *buf = NULL;
        return 0;
}

/*
 * Searches for hamt_ node for given hash
 * If found, returns the node
 * If not found, returns NULL
 */
hamt_n * __find_hamt(hamt_s * s, hamt_n * root, const int hash, const int depth)
{
        if (depth == HAMT_MAX_LEVEL)
                return root;
        if (root == NULL)
                return NULL;

        int logical_index = find_logical_index(hash, depth);
        return __find_hamt(s, root->children[logical_index], hash, depth+1);
         
}

int _find_hamt(hamt_s * s, hamt_n * root, const int hash, 
                const void * key, void ** buf)
{

        hamt_n * leaf = __find_hamt(s, root, hash, 0);
        return _find_hamt_list(s, leaf, key, buf);
}


/*
 * Returns 1 and fills buf with the value asociated with key
 * If key not found, returns 0
 * On error, sets errno and returns -1
 */
int find_hamt(HAMT * H, const void * key, void **buf)
{
        hamt_s * s = (hamt_s *)H;
        if (s->valid != HAMT_VALID) {
                errno = EINVAL;
                return -1;
        }

        int hash = s->info.hash(key);
        
        return _find_hamt(s, s->root, hash, key, buf);
}

int size_hamt(HAMT * H)
{
        hamt_s * s = (hamt_s *)H;
        if (s->valid != HAMT_VALID) {
                errno = EINVAL;
                return -1;
        }
        return s->root->size;
}

void _free_hamt_nodes(hamt_s * s, hamt_n * root)
{
        if (root == NULL)
                return;
        for (int i = 0; i < 32; ++i)
                _free_hamt_nodes(s, root->children[i]);

        _free_hamt_node(s, root);
}

void _free_hamt_node(hamt_s * s, hamt_n * root)
{
        struct hamt_list * it = root->values, * next;
        while (it) {
                next = it->next;
                
                s->info.free_key(it->key);
                s->info.free_elem(it->value);

                free(it);
                it = next;
        }

        free(root);
}

int free_hamt(HAMT * H)
{
        hamt_s * s = (hamt_s *)H;
        if (s->valid != HAMT_VALID) {
                errno = EINVAL;
                return -1;
        }

        _free_hamt_nodes(s, s->root);
        memset(s, 0, sizeof(*s));
        free(s);
        return 0;
}

int _remove_hamt_list(hamt_s * s, hamt_n * node, const void * key, void ** buf)
{
        if (node->values == NULL)
                return HAMT_NOREMOVE;

        struct hamt_list * prev = NULL, * cur = node->values;
        while (cur) {
                if (s->info.cmp_key(cur->key, key) == 0) {
                        /* Match found */
                        if (prev == NULL)
                                node->values = cur->next;
                        else
                                prev->next = cur->next;

                        if (buf != NULL)
                                *buf = s->info.copy_elem(cur->value);

                        s->info.free_elem(cur->value);
                        s->info.free_key(cur->key);
                        free(cur);

                        node->size -= 1;
                        if (node->values == NULL)
                                return HAMT_REMOVECLEAR;
                        else
                                return HAMT_REMOVENOCLEAR;
                }
        }
        return HAMT_NOREMOVE;
}

int _remove_hamt(hamt_s * s, hamt_n * root, const int hash, const int depth,
                const void * key, void **buf)
{
        if (root == NULL)
                return HAMT_NOREMOVE;

        if (depth == HAMT_MAX_LEVEL) {
                int rv = _remove_hamt_list(s, root, key, buf);
                
                if (rv == HAMT_REMOVECLEAR) {
                        _free_hamt_node(s, root);
                }
                return rv;
        }

        int logical_index = find_logical_index(hash, depth);
        int rv = _remove_hamt(s, root->children[logical_index], hash,
                        depth+1, key, buf);
        
        switch (rv) {
                case HAMT_NOREMOVE:
                        return rv;
                case HAMT_REMOVECLEAR:
                        root->size -= 1;
                        root->children[logical_index] = NULL;
                        root->bitfield &= ~(1 << logical_index);
                        if (root->size == 0 && depth != 0) {
                                _free_hamt_node(s, root);
                                return HAMT_REMOVECLEAR;
                        }
                        return HAMT_REMOVENOCLEAR;
                        break;
                case HAMT_REMOVENOCLEAR: 
                        root->size -= 1;
                        return rv;
                        break;
                default:
                        break;

        }
        fprintf(stderr, "Should not reach here at %s: %d", 
                        __FILE__, __LINE__);
        exit(1);
}

/*
 * Returns 1 on success
 *      if buffer != NULL, fills buffer with removed value
 * Returns 0 when key not found
 * Returns -1 on failure (sets errno)
 */
int remove_hamt(HAMT * H, const void * key, void ** buffer)
{
        hamt_s * s = (hamt_s *)H;
        if (s->valid != HAMT_VALID) {
                errno = EINVAL;
                return -1;
        }

        int hash = s->info.hash(key);
        int rv = _remove_hamt(s, s->root, hash, 0, key, buffer);
        return rv;
}
