#ifndef _NBLEI_HAMT_H_
#define _NBLEI_HAMT_H_

typedef void * HAMT;


/*
 * struct hamtinfo is used to initialize HAMT
*/
struct hamtinfo {
        int key_size;
        int elem_size;
        int (*hash)(const void *);            // Hash callback function
        void * (*copy_elem)(const void *);    // Make copy of element (returns pointer)
        int (*free_elem)(void *);       // Frees copy of element (0 for success)
        void * (*copy_key)(const void *);     // Make copy of key (returns pointer)
        int (*free_key)(void *);        // Frees copy of key (0 for success)
        int (*cmp_key)(const void *, const void *);
                                        // Compares two keys.
                                        // Returns 0 if they are the same.
};

HAMT * init_hamt(struct hamtinfo * info);

int insert_hamt(HAMT * H, void * key, void * val);

int find_hamt(HAMT * H, const void * key, void ** buf);

int remove_hamt(HAMT * H, const void * key, void ** buffer);

#endif
