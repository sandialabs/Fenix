#include <mpi.h>
#include "fenix.hpp"
#include "fenix/tasks/request.h"
#include "fenix/logging/comm_log.h"

using namespace fenix::logging;

int MPI_Sendrecv(
  // clang-format off
  const void* sb, int sn, MPI_Datatype sd, int dst, int st,
        void* rb, int rn, MPI_Datatype rd, int src, int rt,
  MPI_Comm comm, MPI_Status* status
  // clang-format on
) {
  if (!comm_log) {
    return PMPI_Sendrecv(
      sb, sn, sd, dst, st, rb, rn, rd, src, rt, comm, status
    );
  } else if (!comm_log.value().is_logging(comm)) {
    MPI_Request req;
    int ret =
      PMPI_Isendrecv(sb, sn, sd, dst, st, rb, rn, rd, src, rt, comm, &req);
    if (ret == MPI_SUCCESS) ret = MPI_Wait(&req, status);
    return ret;
  }

  int ret;
  MPI_Request req;
  ret = comm_log.value().irecv(rb, rn, rd, src, rt, &req);
  if (ret != MPI_SUCCESS) return ret;

  ret = comm_log.value().send(sb, sn, sd, dst, st);
  ret |= MPI_Wait(&req, status);
  return ret;
}

int MPI_Send(
  const void* b, int n, MPI_Datatype d, int dst, int t, MPI_Comm comm
) {
  if (!comm_log) return PMPI_Send(b, n, d, dst, t, comm);
  if (!comm_log.value().is_logging(comm)) {
    MPI_Request req;
    int ret = MPI_Isend(b, n, d, dst, t, comm, &req);
    if (ret == MPI_SUCCESS) ret = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return ret;
  }
  return comm_log.value().send(b, n, d, dst, t);
}

int MPI_Irecv(
  void* b, int n, MPI_Datatype d, int src, int t, MPI_Comm comm, MPI_Request* r
) {
  if (!comm_log || !comm_log.value().is_logging(comm)) {
    return PMPI_Irecv(b, n, d, src, t, comm, r);
  }
  assert(src != MPI_ANY_SOURCE);

  return comm_log.value().irecv(b, n, d, src, t, r);
}

int MPI_Recv(
  void* b, int n, MPI_Datatype d, int src, int t, MPI_Comm comm, MPI_Status* s
) {
  if (!comm_log) return PMPI_Recv(b, n, d, src, t, comm, s);
  MPI_Request r;
  int ret = MPI_Irecv(b, n, d, src, t, comm, &r);
  if (ret == MPI_SUCCESS) ret = MPI_Wait(&r, s);
  return ret;
}

int MPI_Wait(MPI_Request* req, MPI_Status* status) {
  if (!comm_log || *req == MPI_REQUEST_NULL) {
    return PMPI_Wait(req, status);
  }

  fenix::tasks::Status ret;

  for (auto& [rank, log] : comm_log.value().rank_logs) {
    if (log.active_irecv == req) {
      ret = log.wait(req);
      if (status != MPI_STATUS_IGNORE) *status = ret;
      return ret;
    }
  }

  ret = comm_log.value().progress_through(req);
  if (status != MPI_STATUS_IGNORE) *status = ret;
  return ret;
}

int MPI_Barrier(MPI_Comm c) {
  if (!comm_log || !comm_log.value().is_logging(c)) {
    return PMPI_Barrier(c);
  }
  return comm_log->begin<MPI_Barrier>(c);
}

int MPI_Bcast(void* b, int n, MPI_Datatype d, int r, MPI_Comm c) {
  if (!comm_log || !comm_log.value().is_logging(c)) {
    return PMPI_Bcast(b, n, d, r, c);
  }
  return comm_log->begin<MPI_Bcast>(b, n, d, r, c);
}

int MPI_Reduce(
  const void* sb, void* rb, int n, MPI_Datatype d, MPI_Op o, int r, MPI_Comm c
) {
  if (!comm_log || !comm_log.value().is_logging(c)) {
    return PMPI_Reduce(sb, rb, n, d, o, r, c);
  }
  return comm_log->begin<MPI_Reduce>(sb, rb, n, d, o, r, c);
}

int MPI_Allreduce(
  const void* sb, void* rb, int n, MPI_Datatype d, MPI_Op o, MPI_Comm c
) {
  if (!comm_log || !comm_log.value().is_logging(c)) {
    return PMPI_Allreduce(sb, rb, n, d, o, c);
  }
  return comm_log->begin<MPI_Allreduce>(sb, rb, n, d, o, c);
}
