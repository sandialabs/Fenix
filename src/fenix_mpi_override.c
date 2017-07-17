/*
//@HEADER
// ************************************************************************
//
//
//            _|_|_|_|  _|_|_|_|  _|      _|  _|_|_|  _|      _|
//            _|        _|        _|_|    _|    _|      _|  _|
//            _|_|_|    _|_|_|    _|  _|  _|    _|        _|
//            _|        _|        _|    _|_|    _|      _|  _|
//            _|        _|_|_|_|  _|      _|  _|_|_|  _|      _|
//
//
//
//
// Copyright (C) 2016 Rutgers University and Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author Marc Gamell, Eric Valenzuela, Keita Teranishi, Manish Parashar,
//        Rob Van der Wijngaart, and Michael Heroux
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/
#include "fenix_process_recovery.h"
#include "fenix_comm_list.h"
#include <mpi.h>
#include <assert.h>
#include "fenix_ext.h"

static inline 
MPI_Comm __fenix_replace_comm(MPI_Comm comm)
{
    if(__fenix_g_replace_comm_flag &&
       comm == __fenix_g_original_comm &&
       __fenix_g_fenix_init_flag)
        return *__fenix_g_new_world;
    else
        return comm;
}

static inline 
int __fenix_notify_newcomm(int ret, MPI_Comm *newcomm)
{
    if (ret != MPI_SUCCESS || 
        !__fenix_g_fenix_init_flag ||
        *newcomm == MPI_COMM_NULL) return ret;
    ret = PMPI_Comm_set_errhandler(*newcomm, MPI_ERRORS_RETURN);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "[fenix error] Did not manage to set error handler\n");
        PMPI_Comm_free(newcomm);
        ret = MPI_ERR_INTERN;
    } else {
#warning "Calling fenix comm push and fenix init may not have been called... check other places in this function"
        if (__fenix_comm_push(newcomm) != FENIX_SUCCESS) {
            fprintf(stderr, "[fenix error] Did not manage to push communicator\n");
            PMPI_Comm_free(newcomm);
            ret = MPI_ERR_INTERN;
        }
    }
    return ret;
}

// This inlined function is used to avoid a function call for each MPI
// operation call in the case where no failures are detected.
static inline
void __fenix_test_MPI_inline(int ret, const char *msg)
{
    if(ret == MPI_SUCCESS) return;
    __fenix_test_MPI(ret, msg);
}

int MPI_Comm_size(MPI_Comm comm, int *size)
{
    int ret;
    ret = PMPI_Comm_size(__fenix_replace_comm(comm), size);
    __fenix_test_MPI_inline(ret, "MPI_Comm_size");
    return ret;
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
    int ret;
    ret = PMPI_Comm_dup(__fenix_replace_comm(comm), newcomm);
    ret = __fenix_notify_newcomm(ret, newcomm);
    __fenix_test_MPI_inline(ret, "MPI_Comm_dup");
    return ret;
}

int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
    int ret;
    ret = PMPI_Comm_split(__fenix_replace_comm(comm), color, key, newcomm);
    ret = __fenix_notify_newcomm(ret, newcomm);
    __fenix_test_MPI_inline(ret, "MPI_Comm_split");
    return ret;
}

int MPI_Alltoallv(void *sendbuf, int *sendcounts, int *sdispls, MPI_Datatype sendtype, 
                  void *recvbuf, int *recvcounts, int *rdispls, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
    int ret;
    ret = PMPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                         recvcounts, rdispls, recvtype, 
                         __fenix_replace_comm(comm));
    __fenix_test_MPI_inline(ret, "MPI_Alltoallv");
    return ret;
}

int MPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
    int ret;
    ret = PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, 
                         recvtype, __fenix_replace_comm(comm));
    __fenix_test_MPI_inline(ret, "MPI_Allgather");
    return ret;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
    int ret;
    ret = PMPI_Comm_rank(__fenix_replace_comm(comm), rank);
    __fenix_test_MPI_inline(ret, "MPI_Comm_rank");
    return ret;
}


#warning "For OpenMPI 2.0.2, const void * is used!"
#ifdef OPEN_MPI
#define MPI_SEND_BUFF_TYPE void *
#else
#define MPI_SEND_BUFF_TYPE const void *
#endif

int MPI_Allreduce(MPI_SEND_BUFF_TYPE sendbuf, void *recvbuf, int count, 
                  MPI_Datatype type, MPI_Op op, MPI_Comm comm)
{
    int ret;
    ret = PMPI_Allreduce(sendbuf, recvbuf, count, type, op, __fenix_replace_comm(comm));
    __fenix_test_MPI_inline(ret, "MPI_Allreduce");
    return ret;
}

int MPI_Reduce(MPI_SEND_BUFF_TYPE sendbuf, void *recvbuf, int count, MPI_Datatype type,
                  MPI_Op op, int root, MPI_Comm comm)
{
    int ret;
    ret = PMPI_Reduce(sendbuf, recvbuf, count, type, op, root, __fenix_replace_comm(comm));
    __fenix_test_MPI_inline(ret, "MPI_Reduce");
    return ret;
}

int MPI_Barrier(MPI_Comm comm)
{
    int ret;
    ret = PMPI_Barrier(__fenix_replace_comm(comm));
    __fenix_test_MPI_inline(ret, "MPI_Barrier");
    return ret;
}

int MPI_Bcast(void *buf, int count, MPI_Datatype type, int root, MPI_Comm comm)
{
    int ret;
    ret = PMPI_Bcast(buf, count, type, root, __fenix_replace_comm(comm));
    __fenix_test_MPI_inline(ret, "MPI_Bcast");
    return ret;
}

int MPI_Recv(void *buf, int count, MPI_Datatype type, int source, int tag,
             MPI_Comm comm, MPI_Status *status)
{
    int ret;
    ret = PMPI_Recv(buf, count, type, source, tag, __fenix_replace_comm(comm), 
                    status);
    __fenix_test_MPI_inline(ret, "MPI_Recv");
    return ret;
}

int MPI_Send(MPI_SEND_BUFF_TYPE buf, int count, MPI_Datatype type, int dest,
             int tag, MPI_Comm comm)
{
    int ret;
    ret = PMPI_Send(buf, count, type, dest, tag, __fenix_replace_comm(comm));
    __fenix_test_MPI_inline(ret, "MPI_Send");
    return ret;
}

int MPI_Sendrecv(MPI_SEND_BUFF_TYPE sendbuf, int sendcount, 
                 MPI_Datatype sendtype, int dest, int sendtag,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int source, int recvtag,
                 MPI_Comm comm, MPI_Status *status)
{
    int ret;
    ret = PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf,
                        recvcount, recvtype, source, recvtag, 
                        __fenix_replace_comm(comm), status);
    __fenix_test_MPI_inline(ret, "MPI_Sendrecv");
    return ret;
}

#include "fenix_request_store.h"
extern __fenix_request_store_t __fenix_g_request_store;

static inline
void __fenix_override_request(int ret, MPI_Request *request)
{
    if(ret != MPI_SUCCESS) return;

    assert(*request != MPI_REQUEST_NULL);

    // insert 'request' in the request_store
    // get location of 'request' in store and return in 'fenix_request'
    *((int *)request) = __fenix_request_store_add(&__fenix_g_request_store,
						  request);
}

int MPI_Isend(MPI_SEND_BUFF_TYPE buf, int count, MPI_Datatype datatype,
              int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    int ret;
    ret = PMPI_Isend(buf, count, datatype, dest, tag,
                     __fenix_replace_comm(comm), request);
    __fenix_override_request(ret, request);
    __fenix_test_MPI_inline(ret, "MPI_Isend");
    return ret;
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype,
              int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    int ret;
    ret = PMPI_Irecv(buf, count, datatype, source, tag,
                     __fenix_replace_comm(comm), request);
    __fenix_override_request(ret, request);
    __fenix_test_MPI_inline(ret, "MPI_Irecv");
    return ret;
}

int MPI_Wait(MPI_Request *fenix_request, MPI_Status *status)
{
    int ret;
    MPI_Request request = MPI_REQUEST_NULL;
    if(*fenix_request != MPI_REQUEST_NULL)
        __fenix_request_store_get(&__fenix_g_request_store,
				  *((int *) fenix_request),
				  &request);

    ret = PMPI_Wait(&request, status);
    if(ret == MPI_SUCCESS) {
        __fenix_request_store_remove(&__fenix_g_request_store,
				     *((int *) fenix_request));
        assert(request == MPI_REQUEST_NULL);
	*fenix_request = MPI_REQUEST_NULL;
    }
    __fenix_test_MPI_inline(ret, "MPI_Wait");
    return ret;
}

#warning "Fix tabs in source code"

int MPI_Waitall(int count, MPI_Request array_of_fenix_requests[],
                MPI_Status *array_of_statuses)
{
    // The list (array_of_requests) may contain null or inactive handles.
    int ret, i;
    for(i=0 ; i<count ; i++)
        if(array_of_fenix_requests[i] != MPI_REQUEST_NULL)
	    __fenix_request_store_getremove(&__fenix_g_request_store,
					    *((int *)&(array_of_fenix_requests[i])),
					    &(array_of_fenix_requests[i]));

    ret = PMPI_Waitall(count, array_of_fenix_requests, array_of_statuses);
    __fenix_test_MPI_inline(ret, "MPI_Waitall");

    // Requests are deallocated and the corresponding handles in the
    // array are set to MPI_REQUEST_NULL.
    //
    // The function MPI_WAITALL will return in such case the error
    // code MPI_ERR_IN_STATUS and will set the error field of each
    // status to a specific error code. This code will be MPI_SUCCESS,
    // if the specific communication completed; it will be another
    // specific error code, if it failed; or it can be MPI_ERR_PENDING
    // if it has neither failed nor completed. The function
    // MPI_WAITALL will return MPI_SUCCESS if no request had an error,
    // or will return another error code if it failed for other
    // reasons (such as invalid arguments). In such cases, it will not
    // update the error fields of the statuses.
    if(ret != MPI_SUCCESS)
        for(i=0 ; i<count ; i++)
            if((ret != MPI_ERR_IN_STATUS || array_of_statuses[i].MPI_ERROR == MPI_ERR_PENDING)
                && (array_of_fenix_requests[i] != MPI_REQUEST_NULL))
                __fenix_override_request(MPI_SUCCESS, &(array_of_fenix_requests[i]));

    return ret;
}


#warning "Check MPI standard/mpi.h for functions that have MPI_Request"

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
#warning "TODO"
  printf("Fenix: need to implement MPI_Test\n");
}

int MPI_Cancel(MPI_Request *request)
{
#warning "TODO"
  printf("Fenix: need to implement MPI_Cancel\n");
}

int MPI_Request_free(MPI_Request *request)
{
#warning "TODO"
  printf("Fenix: need to implement MPI_Request_free\n");
}
