#include <mpi.h>
#include <string.h>

#include "fenix_stack.h"

/* 

   MPI_REQUEST_NULL = 0;

   user MPI_Irecv(&req);
   fenix PMPI_Irecv(&req);   
   MPI returns req=222;
   fenix store_add(222);
   store returns "-123";
   fenix returns req=-123;

   user MPI_Wait(-123);
   fenix store_get(-123);
   store_get {req_id = 0; req=222;}
   store_get returns req=222;
   fenix PMPI_Wait(222);

 */


typedef struct {
    char valid;
    MPI_Request r;
} __fenix_request_t;

#define __fenix_dynamic_array_type      __fenix_request_t
#define __fenix_dynamic_array_typename  req
#include "fenix_dynamic_array.h"
#undef __fenix_dynamic_array_type
#undef __fenix_dynamic_array_typename

typedef struct {
    __fenix_req_dynamic_array_t reqs; // list of requests
    int first_unused_position;        // first position in 'reqs' that has never been used
    __fenix_stack_t freed_list;       // list of positions in 'reqs' that are not used anymore
} __fenix_request_store_t;

static inline
void __fenix_request_store_init(__fenix_request_store_t *s)
{
    s->first_unused_position = 0;
    __fenix_int_stack_init(&(s->freed_list), 100);
    __fenix_req_dynamic_array_init(&(s->reqs), 500);
}

static inline
void __fenix_request_store_destroy(__fenix_request_store_t *s)
{
    int valid_count = 0, i;
    for(i=0 ; i<s->first_unused_position ; i++)
        if(s->reqs.elements[i].valid) valid_count++;
    if(valid_count > 0)
        printf("[Fenix warning] __fenix_request_store_destroy. store contains valid elements (valid elems %d, first_unused_pos %d)\n", valid_count, s->first_unused_position);
    __fenix_req_dynamic_array_destroy(&(s->reqs));
    __fenix_int_stack_destroy(&(s->freed_list));
    s->first_unused_position = 0;
}

// returns request_id (i.e. position in the s->reqs.elements array)
static inline
int __fenix_request_store_add(__fenix_request_store_t *s, 
                              MPI_Request *r)
{
    assert(*r != MPI_REQUEST_NULL);
    int position = -1;
    if(s->freed_list.size > 0) {
        position = __fenix_int_stack_pop(&(s->freed_list));
    } else {
        position = s->first_unused_position++;
    }
    assert(position >= 0);
    __fenix_req_dynamic_array_inc(&(s->reqs));
    __fenix_request_t *f = &(s->reqs.elements[position]);
    assert(!f->valid);
    memcpy(&(f->r), r, sizeof(MPI_Request));
    f->valid = 1;

    // Cannot return a position that is equivalent to MPI_REQUEST_NULL
    MPI_Request r_test;
    *((int *)&r_test) = position;
    if(r_test == MPI_REQUEST_NULL) {
        position = -123;
	{
	    *((int *)&r_test) = position;
	    assert(r_test != MPI_REQUEST_NULL);
	}
    }
    return position;
}

static inline
void __fenix_request_store_get(__fenix_request_store_t *s, 
                               int request_id,
                               MPI_Request *r)
{
    {
        MPI_Request r_test;
        *((int *)&r_test) = request_id;
        assert(r_test != MPI_REQUEST_NULL);
    }
    if(request_id == -123) {
        MPI_Request r_test = MPI_REQUEST_NULL;
        request_id = *((int*) &r_test);
    }
    __fenix_request_t *f = &(s->reqs.elements[request_id]);
    assert(f->valid);
    memcpy(r, &(f->r), sizeof(MPI_Request));
    assert(*r != MPI_REQUEST_NULL);
}

static inline
void __fenix_request_store_remove(__fenix_request_store_t *s, 
                                  int request_id)
{
    {
        MPI_Request r_test;
        *((int *)&r_test) = request_id;
        assert(r_test != MPI_REQUEST_NULL);
    }
    if(request_id == -123) {
        MPI_Request r_test = MPI_REQUEST_NULL;
        request_id = *((int*) &r_test);
    }
    __fenix_request_t *f = &(s->reqs.elements[request_id]);
    assert(f->valid);
    f->valid = 0;

    __fenix_int_stack_push(&(s->freed_list), request_id);
}


static inline
void __fenix_request_store_getremove(__fenix_request_store_t *s, 
				     int request_id,
				     MPI_Request *r)
{
    {
        MPI_Request r_test;
        *((int *)&r_test) = request_id;
        assert(r_test != MPI_REQUEST_NULL);
    }
    if(request_id == -123) {
        MPI_Request r_test = MPI_REQUEST_NULL;
        request_id = *((int*) &r_test);
    }
    __fenix_request_t *f = &(s->reqs.elements[request_id]);
    assert(f->valid);
    memcpy(r, &(f->r), sizeof(MPI_Request));
    assert(*r != MPI_REQUEST_NULL);
    f->valid = 0;
    __fenix_int_stack_push(&(s->freed_list), request_id);
}

static inline
void __fenix_request_store_waitall_removeall(__fenix_request_store_t *s)
{
    int i;
    for(i=0 ; i<s->first_unused_position ; i++) {
        __fenix_request_t *f = &(s->reqs.elements[i]);
        if(f->valid) {
#warning "What to do with requests upon failure? Wait or Cancel?"
            PMPI_Wait(&(f->r), MPI_STATUS_IGNORE);
            if(i == MPI_REQUEST_NULL)
                __fenix_request_store_remove(s, -123);
            else
                __fenix_request_store_remove(s, i);
        }
    }

    s->first_unused_position = 0;
    __fenix_int_stack_clear(&(s->freed_list));
}
