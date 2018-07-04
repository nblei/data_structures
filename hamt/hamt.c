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

#define find_index(hash, depth) ((hash >> ((depth) * 5)) & 0x01f)

struct hamt_list {
        void * key;
        void * value;
        struct hamt_list * next;
};

typedef struct hamt_node hamt_n;
struct hamt_node {
        struct hamt_list * values;
        uint32_t bitfield;
        struct hamt_node ** arr;
};

typedef struct {
        struct hamtinfo info;
        int size;
        hamt_n * root;
        int valid;
} hamt_s;

/*
 * Find the node associated with the given trie / key combination
 * If node does not exist, returns NULL
 */
hamt_n * find_node(hamt_s * t, const void * key);
/*
 * Find the node associated with the given trie / key combination
 * If node does not exist, it (and all necessary ancestors) is created
 */
hamt_n * find_create(hamt_s * t, const void * key);
hamt_n * _find_create(hamt_n * root, int hash, int depth, char create);
hamt_n * find_create_child(hamt_n * root, int idx, char create);

/*
 * Inserts a key-value list node in the associated list
 */
void insert_key_val(hamt_s * t, struct hamt_list ** head, void * key, void * value);
void remove_key_val(hamt_s * t, struct hamt_list ** head,
                const void * key, void ** buffer);

HAMT * init_hamt(struct hamtinfo * info)
{
        if (info == NULL || info->key_size < 1 ||
                        info->elem_size < 1 || info->hash == NULL ||
                        info->copy_elem == NULL || info->free_elem == NULL)
                return NULL;

        hamt_s * rv = (hamt_s*)calloc(1, sizeof(*rv));
        if (rv == NULL)
                return NULL;
        rv->root = (hamt_n*)calloc(1, sizeof(*(rv->root)));
        if (rv->root == NULL) {
                free(rv);
                return NULL;
        }

        memcpy(&rv->info, info, sizeof(*info));
        rv->size = 0;
        rv->valid = HAMT_VALID;


        return (HAMT*)rv;
}

int insert_hamt(HAMT * H, void * key, void * val)
{
        hamt_s * t = (hamt_s *)H;
        if (t->valid != HAMT_VALID) {
                errno = EINVAL;
                return -1;
        }
        
        hamt_n * node = find_create(t, key);
        insert_key_val(t, &(node->values), key, val);
        return 0;
}

void insert_key_val(hamt_s * t, struct hamt_list ** head,
                void * key, void * value)
{
        struct hamt_list * cur, * prev;
        prev = NULL;
        cur = *head;
        if (cur == NULL) {
                *head = (struct hamt_list *)calloc(1, sizeof *cur);
                if (*head == NULL) {
                        perror("insert_key_val: calloc");
                        exit(1);
                        // TODO this should return an error
                }
                (*head)->key = t->info.copy_key(key);
                (*head)->value = t->info.copy_elem(value);
                return;
        }
        while (cur) {
                if (t->info.cmp_key(cur->key, key) == 0) {
                        t->info.free_elem(cur->value);
                        cur->value = t->info.copy_elem(value);
                        return;
                }
                prev = cur;
                cur = cur->next;
        }

        /* Key not found, so insert new node */
        assert(prev);
        prev->next = (struct hamt_list *)malloc(sizeof *(prev->next));
        prev->next->next = NULL;
        prev->next->key = t->info.copy_key(key);
        prev->next->value = t->info.copy_elem(value);
        return;
}

void remove_key_val(hamt_s * t, struct hamt_list ** head, const void * key, 
                void ** buffer)
{
        struct hamt_list * cur, * prev;
        prev = NULL;
        cur = *head;
        if (cur == NULL)
                return;

        while (cur) {
                if (t->info.cmp_key(cur->key, key) == 0) {
                        if (buffer != NULL)
                                *buffer = t->info.copy_elem(cur->value);
                        t->info.free_elem(cur->value);
                        t->info.free_key(cur->key);
                        if (prev != NULL)
                                prev->next = cur->next;
                        else
                                *head = cur->next;
                        free(cur);
                        return;
                }
                prev = cur;
                cur = cur->next;
        }

        return;
}

hamt_n * _find_create(hamt_n * root, int hash, int depth, char create)
{
        /*
        printf("_find_create:\n\thash: %d\n\tdepth: %d\n\tidx: %d\n",
                        hash, depth, find_index(hash, depth));
                        */
        if (root == NULL)
                return NULL;
        int idx = find_index(hash, depth);
        if (depth == HAMT_MAX_LEVEL)
               return root; 
        return _find_create(find_create_child(root, idx, create),
                        hash, depth+1, create);
}

hamt_n * find_create(hamt_s * t, const void * key)
{
        hamt_n * rv = _find_create(t->root, t->info.hash(key), 0, HAMT_CREATE);
        return rv;
}

/*
 * Finds (creates) idx'th child of rot
 */ 
hamt_n * find_create_child(hamt_n * root, int idx, char create)
{
        assert(root != NULL);
        if ((root->bitfield & (1 << idx)) == 0) {
                if (create == 0)
                        return NULL;
                int nchildren = __builtin_popcount(root->bitfield);
                if (nchildren == 0) {
                        root->arr = (hamt_n **)calloc(1, sizeof(*(root->arr)));
                        if (root->arr == NULL)
                                return NULL;
                }
                else {
                        hamt_n ** temp = (hamt_n**)realloc(root->arr, sizeof(*temp) *
                                        (nchildren + 1));
                        if (temp == NULL) {
                                perror("find_create_child: realloc");
                                return NULL;
                        }
                        else
                                root->arr = temp;
                }

                root->bitfield |= 1 << idx;
                uint32_t bfield = root->bitfield;
                int offset = __builtin_popcount( bfield & (~(0xffffffff << idx)));
                // index offset is where the new pointer goes
                // so move everything from offset to nchildren-1 to the right
                assert(nchildren >= offset);
                memmove((void*)(root->arr + sizeof(*(root->arr)) * offset),
                                (void*)(root->arr + sizeof(*root->arr) * 
                                                (offset + 1)),
                                sizeof(*root->arr) * (nchildren - offset));

                root->arr[offset] = (hamt_n *)calloc(1, sizeof *(root->arr[idx]));
                return root->arr[offset];
        }

        // Node already exists, so we jus need to find it
        uint32_t bfield = root->bitfield;
        int offset = __builtin_popcount( bfield & (~(0xffffffff << idx)));

        return root->arr[offset];
}

int find_hamt(HAMT * H, const void * key, void ** buffer)
{
        hamt_s * h = (hamt_s *)H;
        if (h->valid != HAMT_VALID) {
                errno = EINVAL;
                return -1;
        }
        hamt_n * rv = find_node(h, key);
        if (rv == NULL)
                return -1;
        else {
                // TODO take care of collision case
                struct hamt_list * l = rv->values;
                while (l) {
                        if (0 == h->info.cmp_key(key, l->key)) {
                                *buffer = h->info.copy_elem(l->value);
                                return 0;
                        }
                        l = l->next;
                }
                return -1;
        }

}


hamt_n * find_node(hamt_s * t, const void * key)
{
        hamt_n * rv = _find_create(t->root, t->info.hash(key), 0, HAMT_NOCREATE);
        return rv;
}

int remove_hamt(HAMT * H, const void * key, void ** buffer)
{
        hamt_s * h = (hamt_s *)H;
        if (h->valid != HAMT_VALID) {
                errno = EINVAL;
                return -1;
        }

        int hash = h->info.hash(key);
        hamt_n * ham_stack[HAMT_MAX_LEVEL + 1];
        ham_stack[0] = h->root;
        for (int i = 1; i < HAMT_MAX_LEVEL + 1; ++i) {
                if (ham_stack[i-1] == NULL)
                        return 0;
                int idx = find_index(hash, i-1);

                ham_stack[i] = find_create_child(ham_stack[i-1], idx, HAMT_NOCREATE);
        }

        if (ham_stack[HAMT_MAX_LEVEL] == NULL)
                return 0;
        // At this point, we've reached the 'leaf' node of the Trie
        remove_key_val(h, &(ham_stack[HAMT_MAX_LEVEL]->values), key, buffer);

        return 0;
}

