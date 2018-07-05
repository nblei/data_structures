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

#define find_index(hash, depth) ((hash >> ((depth) * 5)) & 0x01f)

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
void free_nodes(hamt_n * root);
int free_node(hamt_n * node);
int upsize_array(hamt_n * node, int idx);
int downsize_array(hamt_n * node, int idx);

/*
 * Frees a single node, non-recursively
 */
int free_node(hamt_n * node)
{
        if (node == NULL)
                return 1;
        if (node->bitfield != 0)
                return 0;
        if (node->values != NULL)
                return 0;

        /* Node ready to be freed, so free and return 1 */
        free(node->arr);
        node->arr = NULL;
        free(node);
        return 1;
}

void free_nodes(hamt_n * root)
{
        if (root == NULL)
                return;

        /* Free values list */
        struct hamt_list * head = root->values, * next;
        while (head) {
                next = head->next;
                free(head);
                head = next;
        }
        /* Recursively free children */
        int n = __builtin_popcount(root->bitfield);
        for (int i = 0;i < n; ++i) {
                free_nodes(root->arr[i]);
        }
        /* Free children array */
        free(root->arr);
        /* free node */
        free(root);
}

/*
 * Inserts a key-value list node in the associated list
 */
int insert_key_val(hamt_s * t, struct hamt_list ** head, void * key, void * value);
int remove_key_val(hamt_s * t, struct hamt_list ** head,
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
        int rv = insert_key_val(t, &(node->values), key, val);
        t->size += rv;
        return 0;
}

/* Returns 1 if new key was inserted, 0 if existing key had value changed */
int insert_key_val(hamt_s * t, struct hamt_list ** head,
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
                return 1;
        }
        while (cur) {
                if (t->info.cmp_key(cur->key, key) == 0) {
                        t->info.free_elem(cur->value);
                        cur->value = t->info.copy_elem(value);
                        return 0;
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
        return 1;
}

int remove_key_val(hamt_s * t, struct hamt_list ** head, const void * key, 
                void ** buffer)
{
        struct hamt_list * cur, * prev;
        prev = NULL;
        cur = *head;
        if (cur == NULL)
                return HAMT_NOREMOVE;

        while (cur) {
                if (t->info.cmp_key(cur->key, key) == 0) {
                        t->size -= 1;
                        if (buffer != NULL)
                                *buffer = t->info.copy_elem(cur->value);
                        t->info.free_elem(cur->value);
                        t->info.free_key(cur->key);
                        if (prev != NULL)
                                prev->next = cur->next;
                        else
                                *head = cur->next;
                        free(cur);
                        if (*head) {
                                return HAMT_REMOVENOCLEAR;
                        }
                        else
                                return HAMT_REMOVECLEAR;
                }
                prev = cur;
                cur = cur->next;
        }

        return HAMT_NOREMOVE;
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

                int next_idx = upsize_array(root, idx);
                return root->arr[next_idx];
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

int downsize_array(hamt_n * node, int idx)
{
       int arrsize = __builtin_popcount(node->bitfield); 
       int remove_idx = __builtin_popcount(
                node->bitfield & (~(HAMT_MASK32 << idx)));

        // Removing only element in the array
        if (arrsize == 1) {
                free(node->arr);
                node->arr = NULL;
                return 0;
        }

        if (remove_idx < arrsize - 1)
                memmove( node->arr + remove_idx, node->arr + remove_idx + 1,
                        sizeof(*(node->arr)) * (arrsize - 1 - remove_idx));

        node->arr = (hamt_n**)realloc(node->arr,
                        sizeof(*(node->arr)) * (arrsize - 1));

        node->bitfield &= ~(1 << idx);
        return 0;
}

/*
 * Upsizes a node's children array
 * @param node: node
 * @param idx:  logical index (0 - 31) of node's 32-sized pseudo-array
 *              this is the index which gets physically allocated
 * @return: the physical index into the pseudo array corresponding to
 *              the logical index
 */
int upsize_array(hamt_n * node, int idx)
{
        int arrsize = __builtin_popcount(node->bitfield);
        int add_idx = __builtin_popcount(
                node->bitfield & (~(HAMT_MASK32 << idx))
                        );
        
        node->arr = (hamt_n **)realloc(node->arr,
                                        sizeof(*(node->arr)) * (arrsize+1));


        memmove( node->arr + add_idx + 1, node->arr + add_idx,
                sizeof(*(node->arr)) * (arrsize - add_idx));

        node->bitfield |= 1 << idx;
        node->arr[add_idx] = (hamt_n *)calloc(1, sizeof(*(node->arr[add_idx])));
        return add_idx;
}

int _remove_array_index(hamt_n * node, int index)
{
        int arr_size = __builtin_popcount(node->bitfield);
        int arr_idx = __builtin_popcount(
                node->bitfield & (~(HAMT_MASK32 << index)));

        node->bitfield &= ~(1 << index);

        // Removing only element in the array
        if (arr_size == 1) {
                free(node->arr);
                node->arr = NULL;
                return 0;
        }

        // if `index` is first element in the array, then arr_idx = 0
        // Thus, we are dealing with a zero-index array address
        memmove(
                (char*)(node->arr) + sizeof(*(node->arr)) * arr_idx,
                (char*)(node->arr) + sizeof(*(node->arr)) * (arr_idx+1),
                (arr_size - 1 - arr_idx) * sizeof(*(node->arr))
                );

        hamt_n ** temp = (hamt_n**)realloc(node->arr,
                        sizeof(*temp) * (arr_size - 1));
        if (temp == NULL && (arr_size - 1) > 0)
                return 0;
        node->arr = temp;
        return 0;

}

int _remove_hamt(hamt_s * t, hamt_n * node, const int depth, const int hash,
                const void * key, void ** buffer)
{
        if (depth == HAMT_MAX_LEVEL) {
                int rv = remove_key_val(t, &(node->values), key, buffer);
                if (rv == HAMT_REMOVECLEAR)
                        free_node(node);
                return rv;
        }

        int idx = find_index(hash, depth);
        if ((node->bitfield & (1 << idx)) == 0)
                return HAMT_NOREMOVE;

        int arridx = __builtin_popcount( node->bitfield & (~(0xffffffff << idx)));

        int rv = _remove_hamt(t, node->arr[arridx], depth+1, hash, key, buffer);
        if (rv == HAMT_NOREMOVE || rv == HAMT_REMOVENOCLEAR)
                return rv;

        /* Clear bit in bitfield and Realloc the array */

        downsize_array(node, idx);
        
        if (node->bitfield != 0 || depth == 0)
                return HAMT_NOREMOVE;
        free_node(node);
        return HAMT_REMOVECLEAR;
}

int remove_hamt(HAMT * H, const void * key, void ** buffer)
{
        hamt_s * h = (hamt_s *)H;
        if (h->valid != HAMT_VALID) {
                errno = EINVAL;
                return -1;
        }

        int hash = h->info.hash(key);
        _remove_hamt(h, h->root, 0, hash, key, buffer);

        return 0;
}

int size_hamt(HAMT * H)
{
        hamt_s * h = (hamt_s *)H;
        if (h->valid != HAMT_VALID) {
                errno = EINVAL;
                return -1;
        }

        return h->size;
}

int clear_hamt(HAMT * H)
{
        hamt_s * h = (hamt_s *)H;
        if (h->valid != HAMT_VALID) {
                errno = EINVAL;
                return -1;
        }
        free_nodes(h->root);
        h->root = (hamt_n *)calloc(1, sizeof *(h->root));
        h->size = 0;
        return 0;
}

int free_hamt(HAMT * H)
{
        hamt_s * h = (hamt_s *)H;
        if (h->valid != HAMT_VALID) {
                errno = EINVAL;
                return -1;
        }
        free_nodes(h->root);
        memset(h, 0, sizeof(*h));
        free(h);
        return 0;
}
