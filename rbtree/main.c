#include "rbtree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

int int_copy(void * dest, void * src);
int int_comp(void * p, void * q);
int int_free(void * p);

int test_insert_remove(int n);

int main(int argc, char ** argv)
{
        if (argc != 2) {
                fprintf(stderr, "Usage: main <number of iterations>\n");
                exit(1);
        }

        int n = atoi(argv[1]);


        test_insert_remove(n);

        exit(EXIT_SUCCESS);
}

int test_insert_remove(int n)
{

        struct rbtreeinfo info = {
                .keycopy = int_copy,
                .keycomp = int_comp,
                .keyfree = int_free,
        };
        RBTREE * tree = rb_init(&info);
        assert(tree);

        rb_insert(tree, (void *)(uintptr_t)5, NULL);
        rb_insert(tree, (void *)(uintptr_t)6, NULL);
        rb_insert(tree, (void *)(uintptr_t)15, NULL);
        rb_insert(tree, (void *)(uintptr_t)2, NULL);
        rb_insert(tree, (void *)(uintptr_t)8, NULL);
        rb_insert(tree, (void *)(uintptr_t)3, NULL);
        rb_insert(tree, (void *)(uintptr_t)125, NULL);

        assert(rb_size(tree) == 7);
        assert(rb_has(tree, (void*)(uintptr_t)6));
        rb_remove(tree, (void *)(uintptr_t)6);
        assert(!rb_has(tree, (void*)(uintptr_t)6));

        assert(rb_size(tree) == 6);
        assert(rb_has(tree, (void*)(uintptr_t)8));
        rb_remove(tree, (void *)(uintptr_t)8);
        assert(!rb_has(tree, (void*)(uintptr_t)8));

        assert(rb_size(tree) == 5);
        assert(rb_has(tree, (void*)(uintptr_t)125));
        rb_remove(tree, (void *)(uintptr_t)125);
        assert(!rb_has(tree, (void*)(uintptr_t)125));

        assert(rb_size(tree) == 4);
        assert(rb_has(tree, (void*)(uintptr_t)2));
        assert(rb_has(tree, (void*)(uintptr_t)3));
        assert(rb_has(tree, (void*)(uintptr_t)5));
        assert(rb_has(tree, (void*)(uintptr_t)15));

        rb_free(tree);


        tree = NULL;
        tree = rb_init(&info);
        assert(tree);

        for (int i = 0; i < n; ++i) {
                assert(rb_size(tree) == i);
                assert(!rb_has(tree, (void*)(uintptr_t)i));
                rb_insert(tree, (void *)(uintptr_t)i, NULL);
                assert(rb_has(tree, (void*)(uintptr_t)i));
        }

        for (int i = 0; i < n; ++i) {
                if (rb_size(tree) != (n - i)) {
                        fprintf(stderr, "Expected %d nodes, but only has %d"
                                        " nodes.\n", n-i, rb_size(tree));
                }
                assert(rb_has(tree, (void*)(uintptr_t)i));
                rb_remove(tree, (void*)(uintptr_t)i);
                assert(!rb_has(tree, (void*)(uintptr_t)i));
        }

        rb_free(tree);
        return 0;
}

int int_copy(void * dest, void * src)
{
        int * d = (int *)dest;
        int * s = (int *)src;
        *d = *s;
        return 0;
}

int int_comp(void * p, void * q)
{
        int a = (int)(uintptr_t)p;
        int b = (int)(uintptr_t)q;
        if (a == b)
                return 0;
        else if (a < b)
                return -1;
        else
                return 1;
}

int int_free(void * p)
{
        (void)p;
        return 0;
}
