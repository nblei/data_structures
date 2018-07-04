#include "trie.h"
#include <errno.h>
#include <stdlib.h>

/* Make a trie to for an alphabet of size width at depth dpeth*/
/* On failure, returns NULL (sets ERRNO) */
static TRIE * _make_trie(int depth);


TRIE * make_trie(void)
{
        return _make_trie(0);
}


static TRIE * _make_trie(int depth)
{
        TRIE * rv = calloc(1, sizeof(*rv));
        if (rv == NULL)
                return rv;

        rv->depth = depth;
        return rv;
}

int add_word_trie(TRIE * trie, const char * word)
{
        if (trie == NULL) {
                errno = EINVAL;
                return -1;
        }

        if (word[0] == '\0') {
                trie->in_dict = IN_TRIE;
                return 0;
        }

        int idx = word[0] - 'a';

        if (trie->children[idx] == NULL) {
                trie->children[idx] = _make_trie(trie->depth + 1);
                if (trie->children[idx] == NULL)
                        return -1;
        }

        return add_word_trie(trie->children[idx], word+1);
}

int search_trie(TRIE * root, const char * word)
{
        if (NULL == root)
                return 0;

        if (word[0] == '\0')
                return root->in_dict == IN_TRIE ? 1 : 0;

        int idx = word[0] - 'a';
        
        return search_trie(root->children[idx], word+1);
}
