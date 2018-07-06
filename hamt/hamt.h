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

/**
 * @description: Initializes a heap array mapped trie
 * @param info: a filled out struct hamtinfo
 * @return: A pointer to a HAMT.  On error, returns NULL (sets errno)
 **/
HAMT * init_hamt(struct hamtinfo * info);

/**
 * @description: Inserts 'val' at 'key' in 'H'
 * @param H: The HAMT to insert into
 * @param key: The key associated with 'val'
 * @param val: The value/element inserting into the HAMT
 * @return: Returns 1 if a value associated with a new key is inserted.
 *              returns 0 if the key already exists (changes the value)
 *              returns -1 on error (sets errno).
 **/
int insert_hamt(HAMT * H, void * key, void * val);

/**
 * @description: Finds the value associated with 'key' and copies it into
 *                      'buf'
 * @param H: The HAMT to find in
 * @param key: The key associated with a value
 * @param buf: Buffer to copy value into (user must ensure buffer is
 *                      appropriately sized based on copy_elem/elem size
 *                      parameters of hamtinfo)
 * @return: If key found, returns 1.  If key not found, returns 0.  On error,
 *              returns -1 (sets errno).
 **/
int find_hamt(HAMT * H, const void * key, void ** buf);

/**
 * @description: Removes the 'key'/value pair and optionally copies the
 *                      value into 'buf'
 * @param H:   The HAMT to remove from
 * @param key: The key to remove from 'H'
 * @param buf: Buffer to copy value into (user must ensure buffer is
 *                      appropriately sized based on copy_elem/elem size
 *                      parameters of hamtinfo) or NULL, if value isn't
 *                      needed
 * @return: If key found and item removed, returns 1.
 *         If key not found, returns 0.  On error, returns -1 (sets errno).
 **/
int remove_hamt(HAMT * H, const void * key, void ** buffer);

/**
 * @description: Returns the size of the HAMT, where size is the number of
 *               distinct key/value pairs
 * @param H:     The HAMT whose size is wanted
 * @return:      The size of the 'H'
 */
unsigned int size_hamt(HAMT * H);

/**
 * @description: Removes all key/value pairs from the HAMT, 'H'
 * @param H: The HAMT to clear
 * @return: 0 on success, -1 on failure (sets errno)
 **/
int clear_hamt(HAMT * H);

/**
 * @description: Frees all memory associated with HAMT 'H'
 * @param H: The HAMT to free
 * @return: 0 on success, -1 on failure (sets errno)
 **/
int free_hamt(HAMT * H);
#endif
