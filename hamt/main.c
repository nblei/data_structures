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


int main(void)
{
        struct hamtinfo info = {
                .key_size = sizeof( char *),
                .elem_size = 4,
                .hash = hash_str,
                .copy_elem = copy_int,
                .free_elem = free_int,
                .copy_key = copy_str,
                .free_key = free_str,
                .cmp_key = comp_str
        };
        HAMT * h = init_hamt(&info);

        int pows = 16;
        char buffer[20];
        for (volatile int i = 0; i < (1 << pows); ++i) {
                sprintf(buffer, "%d", i);
                insert_hamt(h, (void*)buffer, (void*)(uintptr_t)i);
        }

        printf("size at peak: %d\n", size_hamt(h));

        for (volatile int i = 0; i < (1 << pows); ++i) {
                sprintf(buffer, "%d", i);
                uintptr_t buf = -1;
                remove_hamt(h, (void*)buffer, (void**)&buf);
                if ((int)buf != i) {
                        printf("Expected %d, but got %d\n", i, (int)buf);
                }
        }


        printf("size after remove: %d\n", size_hamt(h));

        free_hamt(h);

        //free_hamt(h);
        exit(EXIT_SUCCESS);
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

