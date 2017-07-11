#define __fenix_dynamic_array_type     int
#define __fenix_dynamic_array_typename int_stack
#include "fenix_dynamic_array.h"
#undef __fenix_dynamic_array_type
#undef __fenix_dynamic_array_typename

typedef __fenix_int_stack_dynamic_array_t __fenix_stack_t;

static inline
void __fenix_int_stack_init(__fenix_stack_t *s,
                            int max_size)
{
    __fenix_int_stack_dynamic_array_init(s, max_size);
}

static inline
void __fenix_int_stack_destroy(__fenix_stack_t *s)
{
    __fenix_int_stack_dynamic_array_destroy(s);
}

static inline
void __fenix_int_stack_push(__fenix_stack_t *s,
                            int element)
{
    __fenix_int_stack_dynamic_array_inc(s);
    s->elements[s->size-1] = element;
}

static inline
int __fenix_int_stack_pop(__fenix_stack_t *s)
{
    int ret = s->elements[s->size-1];
    __fenix_int_stack_dynamic_array_dec(s);
    return ret;
}

