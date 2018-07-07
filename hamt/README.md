## Hash Array Mapped Trie (HAMT) version 0.1

A generic implementation of a Hash Array Mapped Trie in C

A Hash Array Mapped Trie provides constant time insertion, search, and removal of key-value pairs.
First, the key is hashed to a 32-bit integer.
The hashed integer is used to navigate a 32-ary Trie
(5 bits of the hash are used to determine the next child).
Key-value pairs are stored in a linked list at each leaf node.  Since each leaf node is
associated with exactly one of the 2^32 possible hashes, the HAMT is very resilient to hash collisions.
For example, if the key-space consists of all 32-bit integers, then the identity map, used as a hash,
will ensure that hash collisions are impossible.
In other words, the HAMT is as resilient to hash collisions as a hash table with 2^32 entries, yet takes
up space which is proportional to the number of key-values stored in the HAMT.

Additionally, a HAMT is one of the few data structures which becomes quicker to access and insert into
as it grows (remove time doesn't change), since the expected number of nodes malloced on insertion decreases
as the size of the HAMT increases.  This affect can be counteracted by HASH collisions, althogh HASH
collisions are impossible for some key-types (such as 32-bit integers).

### Supports

+ Generic types for keys and values
+ User specified hashing function
+ Collision handling

### Interface

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
