#include "hamt.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

void * copy_int(const void *);
int free_int(void *);
int comp_int(const void *, const void *);
int hash_int(const void *);
int hash_str(const void * str);
void * copy_str(const void * str);
int free_str(void * str);
int comp_str(const void * a, const void *b); 
int string_int_test(int pows);
int int_int_test(int pows);


int main(void)
{
        
        string_int_test(20);
        int_int_test(16);
        exit(EXIT_SUCCESS);
}

int int_int_test(int pows)
{
        printf("Beginning test\n\tKey: int\n\tValue: int\n"
               "\tMax Size: %d\n", 1 << pows);

        struct hamtinfo info = {
                .key_size = sizeof(int),
                .elem_size = sizeof(int),
                .hash = hash_int,
                .copy_elem = copy_int,
                .free_elem = free_int,
                .copy_key = copy_int,
                .free_key = free_int,
                .cmp_key = comp_int
        };
        HAMT * h = init_hamt(&info);
        assert(h);

        uintptr_t buf;
        for (volatile int i = 0; i < (1 << pows); ++i) {
                int rv = insert_hamt(h, (void*)(uintptr_t)i, (void*)(uintptr_t)i);
                assert(rv == 1);
        }

        assert(size_hamt(h) == (1 << pows));

        for (volatile int i = 0; i < (1 <<pows); ++i) {
                find_hamt(h, (void*)(uintptr_t)i, (void**)&buf);
                assert(buf == i);
        }

        assert(size_hamt(h) == (1 << pows));

        for (volatile int i = 0; i < (1 <<pows); ++i) {
                int rv = insert_hamt(h, (void*)(uintptr_t)i, (void*)(uintptr_t)(3*i));
                assert(rv == 0);
        }

        assert(size_hamt(h) == (1 << pows));

        for (volatile int i = 0; i < (1 <<pows); ++i) {
                int rv = remove_hamt(h, (void*)(uintptr_t)i, (void**)&buf);
                assert(rv == 1);
                assert(buf == (3 * i));
        }

        assert(size_hamt(h) == 0);
        free_hamt(h);
        printf("Test Successfull\n\n");
        return 0;
}

int string_int_test(int pows)
{
        printf("Beginning test\n\tKey: char *\n\tValue: int\n\t"
                "Max Size: %d\n", 1 << pows);
        struct hamtinfo info = {
                .key_size = sizeof( char *),
                .elem_size = sizeof( int ),
                .hash = hash_str,
                .copy_elem = copy_int,
                .free_elem = free_int,
                .copy_key = copy_str,
                .free_key = free_str,
                .cmp_key = comp_str
        };
        HAMT * h = init_hamt(&info);
        assert(h);

        char buffer[20];
        for (volatile int i = 0; i < (1 << pows); ++i) {
                sprintf(buffer, "%d", i);
                insert_hamt(h, (void*)buffer, (void*)(uintptr_t)i);
        }

        assert(size_hamt(h) == 1 << pows);

        for (volatile int i = 0; i < (1 << pows); ++i) {
                sprintf(buffer, "%d", i);
                uintptr_t buf = -1;
                remove_hamt(h, (void*)buffer, (void**)&buf);
                assert((int)buf == i);
        }

        assert(size_hamt(h) == 0);
        free_hamt(h);
        printf("Test Successfull\n\n");
        return 0;
}


void * copy_str(const void * str)
{
        const char * s = (const char *)str;
        int l = strlen(s);
        char * rv = (char *)malloc(l + 1); assert(rv);
        rv[l] = '\0';
        strncpy(rv, s, l);
        return (void*)rv;
}

int hash_str(const void * str)
{
        int rv = 0;
        const char * s = (const char *)str;
        int n = strlen(s);
        int m = 1;
        for (int i = 0; i < n; ++i)
        {
                rv += s[i] * m;
                m *= 31;
        }
        return rv;
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

int free_str(void * str)
{
        char * s = (char *)str;
        free(s);
        return 0;
}

int comp_str(const void * a, const void *b)
{
        return strcmp((char *)a, (char *)b);
}

int comp_int(const void * a, const void * b)
{
        return (int*)a == (int*)b ? 0 : 1;
}

int hash_int(const void * num)
{
        return (uintptr_t)num;
}

