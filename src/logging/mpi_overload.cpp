#include <mpi.h>
#include "fenix.hpp"
#include "fenix/tasks/request.hpp"
#include "fenix/logging/message_logging.h"

using namespace fenix;
using namespace fenix::logging;

int MPI_Sendrecv(
  // clang-format off
  const void* sb, int sn, MPI_Datatype sd, int dst, int st,
        void* rb, int rn, MPI_Datatype rd, int src, int rt,
  MPI_Comm comm, MPI_Status* status
  // clang-format on
) {
  if (!impl::is_logging(comm)) {
    return PMPI_Sendrecv(
      sb, sn, sd, dst, st, rb, rn, rd, src, rt, comm, status
    );
  }

  int ret;
  MPI_Request req;
  ret = fenix_rt.active_mlog->irecv(rb, rn, rd, src, rt, &req);
  if (ret != MPI_SUCCESS) return ret;

  ret = fenix_rt.active_mlog->send(sb, sn, sd, dst, st);
  ret |= MPI_Wait(&req, status);
  return ret;
}

int MPI_Send(
  const void* b, int n, MPI_Datatype d, int dst, int t, MPI_Comm comm
) {
  if (!impl::is_logging(comm)) return PMPI_Send(b, n, d, dst, t, comm);
  return fenix_rt.active_mlog->send(b, n, d, dst, t);
}

int MPI_Irecv(
  void* b, int n, MPI_Datatype d, int src, int t, MPI_Comm comm, MPI_Request* r
) {
  if (!impl::is_logging(comm)) {
    return PMPI_Irecv(b, n, d, src, t, comm, r);
  }
  assert(src != MPI_ANY_SOURCE);

  return fenix_rt.active_mlog->irecv(b, n, d, src, t, r);
}

int MPI_Recv(
  void* b, int n, MPI_Datatype d, int src, int t, MPI_Comm comm, MPI_Status* s
) {
  if (!impl::is_logging(comm)) return PMPI_Recv(b, n, d, src, t, comm, s);
  MPI_Request r;
  int ret = MPI_Irecv(b, n, d, src, t, comm, &r);
  if (ret == MPI_SUCCESS) ret = MPI_Wait(&r, s);
  return ret;
}

int MPI_Wait(MPI_Request* req, MPI_Status* status) {
  if (!fenix_rt.active_mlog || *req == MPI_REQUEST_NULL) {
    return PMPI_Wait(req, status);
  }

  fenix::tasks::Status ret;

  for (auto& [rank, log] : fenix_rt.active_mlog->rank_logs) {
    if (log.active_irecv == req) {
      ret = log.wait(req);
      if (status != MPI_STATUS_IGNORE) *status = ret;
      return ret;
    }
  }

  ret = fenix_rt.active_mlog->progress_through(req);
  if (status != MPI_STATUS_IGNORE) *status = ret;
  return ret;
}

int MPI_Barrier(MPI_Comm c) {
  if (!impl::is_logging(c)) {
    return PMPI_Barrier(c);
  }
  return fenix_rt.active_mlog->begin<MPI_Barrier>(c);
}

int MPI_Bcast(void* b, int n, MPI_Datatype d, int r, MPI_Comm c) {
  if (!impl::is_logging(c)) {
    return PMPI_Bcast(b, n, d, r, c);
  }
  return fenix_rt.active_mlog->begin<MPI_Bcast>(b, n, d, r, c);
}

int MPI_Reduce(
  const void* sb, void* rb, int n, MPI_Datatype d, MPI_Op o, int r, MPI_Comm c
) {
  if (!impl::is_logging(c)) {
    return PMPI_Reduce(sb, rb, n, d, o, r, c);
  }
  return fenix_rt.active_mlog->begin<MPI_Reduce>(sb, rb, n, d, o, r, c);
}

int MPI_Allreduce(
  const void* sb, void* rb, int n, MPI_Datatype d, MPI_Op o, MPI_Comm c
) {
  if (!impl::is_logging(c)) {
    return PMPI_Allreduce(sb, rb, n, d, o, c);
  }
  return fenix_rt.active_mlog->begin<MPI_Allreduce>(sb, rb, n, d, o, c);
}
