#include <stdlib.h>
#include <string.h>

#define CAT__(a,b,c) a##b##c
#define CAT_(a,b,c) CAT__(a,b,c)
#define NAME(a) CAT_(__fenix_,__fenix_dynamic_array_typename,a)

#define __fenix_dynamic_array_t       NAME(_dynamic_array_t)
#define __fenix_dynamic_array_init    NAME(_dynamic_array_init)
#define __fenix_dynamic_array_destroy NAME(_dynamic_array_destroy)
#define __fenix_dynamic_array_inc     NAME(_dynamic_array_inc)
#define __fenix_dynamic_array_dec     NAME(_dynamic_array_dec)

typedef struct {
    int size;         // number of used elements in 'elements'
    int max_size;     // number of elements that can fit in 'elements'
    size_t bytes_allocated; // size, in bytes, of 'elements'
    __fenix_dynamic_array_type *elements;
                      // pointer to array where the elements are stored
} __fenix_dynamic_array_t;

static inline 
void __fenix_dynamic_array_init(__fenix_dynamic_array_t *d, 
                                int max_size)
{
    d->size = 0;
    d->max_size = max_size;
    d->bytes_allocated = max_size*sizeof(__fenix_dynamic_array_type);
    d->elements = malloc(d->bytes_allocated);
    memset(d->elements, 0x00, d->bytes_allocated);
}

static inline 
void __fenix_dynamic_array_destroy(__fenix_dynamic_array_t *d)
{
    free(d->elements);
    d->size = 0;
    d->max_size = 0;
}

static inline
void __fenix_dynamic_array_inc(__fenix_dynamic_array_t *d)
{
    assert(d->size <= d->max_size);
    if(d->size == d->max_size) {
        size_t olds = d->bytes_allocated;
        d->bytes_allocated *= 2;
        d->max_size *= 2;
        d->elements = realloc(d->elements, d->bytes_allocated);
        memset(((char *)d->elements)+olds, 0x00, olds);
    }
    d->size++;
}

static inline
void __fenix_dynamic_array_dec(__fenix_dynamic_array_t *d)
{
    assert(d->size > 0);
    d->size--;
}


#undef CAT__
#undef CAT_
#undef NAME
#undef __fenix_dynamic_array_t
#undef __fenix_dynamic_array_init
#undef __fenix_dynamic_array_destroy
#undef __fenix_dynamic_array_inc
#undef __fenix_dynamic_array_dec
