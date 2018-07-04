#ifndef _TRIE_H_
#define _TRIE_H_

#define IN_TRIE 1
#define NOT_IN_TRIE 0

typedef struct trie_node TRIE;
struct trie_node {
        struct trie_node * children[26];    /* Children array */
        int depth;                        /* Node dpeth */
        int in_dict;                      /* Is a terminal node (not leaf) */
};

/* Make a trie */
/* On failure, returns NULL (sets ERRNO) */
TRIE * make_trie(void);

/* Inserts word into trie */
/* On success returns 0.  On failure, returns -1, sets errno */
int add_word_trie(TRIE * trie, const char * word);

/* Searchs trie for word */
/* If found, returns 1.  If not found, returns 0.  On error, returns errno */
int search_trie(TRIE * root, const char * word);

#endif
