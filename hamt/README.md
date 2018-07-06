## Hash Array Mapped Trie (HAMT)

Supports

+ Generic types for keys and values
+ User specified hashing function
+ Collision handling

Interface

```C
struct hamtinfo {
        int key_size;
        int elem_size;
        int (*hash)(const void *);              // Hash callback function
        void * (*copy_elem)(const void *);      // Make copy of element (returns pointer)
        int (*free_elem)(void *);               // Frees copy of element (0 for success)
        void * (*copy_key)(const void *);       // Make copy of key (returns pointer)
        int (*free_key)(void *);                // Frees copy of key (0 for success)
        int (*cmp_key)(const void *, const void *);
                                                // Compares two keys.
                                                // Returns 0 if they are the same.
};

HAMT * init_hamt(struct hamtinfo * info)
int insert_hamt(HAMT * H, void * key, void * val)
int find_hamt(HAMT * H, const void * key, void ** buf)
int remove_hamt(HAMT * H, const void * key, void ** buffer)
unsigned int size_hamt(HAMT * H)
int clear_hamt(HAMT * H)
int free_hamt(HAMT * H)
```
