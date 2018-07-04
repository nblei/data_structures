#include "trie.h"
#include <stdio.h>

#define DICTSZ 2
#define DICTSZ2 3
const char * dict[DICTSZ] = {
        "hello",
        "world"
};
const char * dict2[DICTSZ2] = {
        "hello",
        "world",
        "buddabing"
};

int main(void)
{
        TRIE * trie = make_trie();
        for (int i = 0; i < DICTSZ; ++i)
                add_word_trie(trie, dict[i]);
        
        for (int i = 0; i < DICTSZ2; ++i)
                printf("Trie contains %s? %s\n", dict2[i],
                                search_trie(trie, dict2[i]) ? "Yes" : "No");


}
