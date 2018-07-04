#include "hamt.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

void * copy_int(const void *);
int free_int(void *);
int comp_int(const void *, const void *);
int hash_int(const void *);

int main(void)
{
        struct hamtinfo info = { .key_size = 4,
                .elem_size = 4,
                .hash = hash_int,
                .copy_elem = copy_int,
                .free_elem = free_int,
                .copy_key = copy_int,
                .free_key = free_int,
                .cmp_key = comp_int
        };
        HAMT * h = init_hamt(&info);

        for (int i = 0; i < (1 << 10); ++i) {
                insert_hamt(h, (void*)(uintptr_t)i, (void*)(uintptr_t)i);
        }

        //insert_hamt(h, (void*)(uintptr_t)0, (void*)(uintptr_t)0);
        //insert_hamt(h, (void*)(uintptr_t)0, (void*)(uintptr_t)32);

        for (int i = 0; i < 8; ++i) {
                remove_hamt(h, (void*)(uintptr_t)(i+16), NULL);
        }
        remove_hamt(h, (void*)(uintptr_t)0, NULL);

        int k = 0;
        for (int i = k; i < k+64; ++i) {
                int rv;
                uintptr_t buf;
                rv = find_hamt(h, (void*)(uintptr_t)i, (void**)&buf);
                if (rv == 0) {
                        printf("key: %d, value: %lu\n", i, buf);
                }
                else
                        printf("Error: Key '%d' not found\n", i);
        }
        exit(EXIT_SUCCESS);
}

void * copy_int(const void * num)
{
        uintptr_t k = (uintptr_t)num;
        return (void*)k;
}

int free_int(void * num)
{
        (void)num;
        return 0;
}

int comp_int(const void * a, const void * b)
{
        return (int*)a == (int*)b ? 0 : 1;
}

int hash_int(const void * num)
{
        return (uintptr_t)num % 8;
}

