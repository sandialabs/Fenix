#ifndef FENIX_MPIXX_TASKS_HPP
#define FENIX_MPIXX_TASKS_HPP

#include <type_traits>
#include <utility>
#include <vector>

#include <mpi.h>

#include "fenix/mpixx/status.hpp"
#include "fenix/mpixx/request.hpp"
#include "fenix/mpixx/util.hpp"
#include "fenix/tasks/task.hpp"

namespace fenix::mpixx {

using MPITask = fenix::tasks::Task<Status>;

template <typename T>
MPITask recv(T* b, int n, MPI_Datatype d, int r, int t, MPI_Comm c) {
  MPI_Request request;
  Status ret = MPI_Irecv(b, n, d, r, t, c, &request);
  if (ret) ret = co_await request;
  co_return ret;
}
template <typename T>
auto recv(T* b, int n, int r, int t, MPI_Comm c) {
  return recv(b, datatype_count(b, n), datatype(b), r, t, c);
}
template <typename T>
auto recv(T& b, int r, int t, MPI_Comm c) {
  return recv(&b, 1, r, t, c);
}
template <typename T, typename A>
auto recv(std::vector<T, A>& v, int r, int t, MPI_Comm c) {
  return recv(v.data(), v.size(), r, t, c);
}

template <typename T>
MPITask send(const T* b, int n, MPI_Datatype d, int r, int t, MPI_Comm c) {
  MPI_Request request;
  Status ret = MPI_Isend(b, n, d, r, t, c, &request);
  if (ret) ret = co_await request;
  co_return ret;
}
template <typename T>
auto send(const T* b, int n, int r, int t, MPI_Comm c) {
  return send(b, datatype_count(b, n), datatype(b), r, t, c);
}
template <typename T>
auto send(const T& b, int r, int t, MPI_Comm c) {
  return send(&b, 1, r, t, c);
}
template <typename T, typename A>
auto send(const std::vector<T>& v, int r, int t, MPI_Comm c) {
  return send(v.data(), v.size(), r, t, c);
}

template <typename ST, typename RT>
MPITask sendrecv(
  const ST* sb, int sn, MPI_Datatype sd, int sr, int st,
        RT* rb, int rn, MPI_Datatype rd, int rr, int rt, MPI_Comm c
) {
  auto recv_task = recv(rb, rn, rd, rr, rt, c);
  // ensure lazily-evaluated recv_task actually begins
  recv_task.resume();
  co_await send(sb, sn, sd, sr, st, c);
  co_return co_await recv_task;
}
template <typename ST, typename RT>
auto sendrecv(
  const ST* sb, int sn, int sr, int st,
        RT* rb, int rn, int rr, int rt, MPI_Comm c
) {
  return sendrecv(
    sb, datatype_count(sb, sn), datatype(sb), sr, st,
    rb, datatype_count(rb, rn), datatype(rb), rr, rt, c
  );
}
template <typename ST, typename RT>
auto sendrecv(
  const ST& sb, int sr, int st,
        RT& rb, int rr, int rt, MPI_Comm c
) {
  return sendrecv(&sb, 1, sr, st, &rb, 1, rr, rt, c);
}
template <typename ST, typename SA, typename RT, typename RA>
auto sendrecv(
  const std::vector<ST, SA>& sv, int sr, int st,
        std::vector<RT, RA>& rv, int rr, int rt, MPI_Comm c
) {
  return sendrecv(&sv[0], sv.size(), sr, st, &rv[0], rv.size(), rr, rt, c);
}

template <typename T>
MPITask allreduce(
  const void* sb, T& rb, int n, MPI_Datatype d, MPI_Op o, MPI_Comm c
) {
  MPI_Request request;
  Status ret = MPI_Iallreduce(sb, &rb, n, d, o, c, &request);
  if (ret) ret = co_await request;
  co_return ret;
}
template <typename T>
auto allreduce(const T* sb, T& rb, int n, MPI_Op o, MPI_Comm c) {
  return allreduce(sb, rb, datatype_count(sb, n), datatype(sb), o, c);
}
template <typename T>
auto allreduce(const T& sb, T& rb, MPI_Op o, MPI_Comm c) {
  return allreduce(&sb, rb, 1, o, c);
}
template <typename T, typename A>
auto allreduce(const std::vector<T, A>& sv, T& rb, MPI_Op o, MPI_Comm c) {
  return allreduce(&sv[0], rb, sv.size(), o, c);
}

// Template for pointer types - pass pointer directly to MPI
template <typename T>
MPITask reduce(
  const void* sb, T* rb, int n, MPI_Datatype d, MPI_Op o, int r, MPI_Comm c
) {
  MPI_Request request;
  Status ret = MPI_Ireduce(sb, rb, n, d, o, r, c, &request);
  if (ret) ret = co_await request;
  co_return ret;
}

// Template for reference types - take address of reference
template <typename T>
MPITask reduce(
  const void* sb, T& rb, int n, MPI_Datatype d, MPI_Op o, int r, MPI_Comm c
) {
  MPI_Request request;
  Status ret = MPI_Ireduce(sb, &rb, n, d, o, r, c, &request);
  if (ret) ret = co_await request;
  co_return ret;
}
template <typename T>
auto reduce(const T* sb, T& rb, int n, MPI_Op o, int r, MPI_Comm c) {
  return reduce(sb, rb, datatype_count(sb, n), datatype(sb), o, r, c);
}
template <typename T>
auto reduce(const T& sb, T& rb, MPI_Op o, int r, MPI_Comm c) {
  return reduce(&sb, rb, 1, o, r, c);
}
template <typename T, typename A>
auto reduce(const std::vector<T, A>& sv, T& rb, MPI_Op o, int r, MPI_Comm c) {
  return reduce(&sv[0], rb, sv.size(), o, r, c);
}

template <typename T>
MPITask bcast(T* b, int n, MPI_Datatype d, int r, MPI_Comm c) {
  MPI_Request request;
  Status ret = MPI_Ibcast(b, n, d, r, c, &request);
  if (ret) ret = co_await request;
  co_return ret;
}
template <typename T>
auto bcast(T* b, int n, int r, MPI_Comm c) {
  return bcast(b, datatype_count(b, n), datatype(b), r, c);
}
template <typename T>
auto bcast(T& b, int r, MPI_Comm c) {
  return bcast(b, 1, r, c);
}
template <typename T, typename A>
auto bcast(std::vector<T, A>& v, int r, MPI_Comm c) {
  return bcast(&v[0], v.size(), r, c);
}

template <typename ST, typename RT>
MPITask allgather(
  const ST* sb, int sn, MPI_Datatype sd, RT* rb, int rn, MPI_Datatype rd,
  MPI_Comm c
) {
  MPI_Request request;
  Status ret = MPI_Iallgather(sb, sn, sd, rb, rn, rd, c, &request);
  if (ret) ret = co_await request;
  co_return ret;
}
template <typename ST, typename RT>
auto allgather(const ST* sb, int sn, RT* rb, int rn, MPI_Comm c) {
  return allgather(
    sb, datatype_count(sb, sn), datatype(sb), rb, datatype_count(rb, rn),
    datatype(rb), c
  );
}

template <typename ST, typename RT>
MPITask allgatherv(
  const ST* sb, int sn, MPI_Datatype sd, RT* rb, const int* rn,
  const int* displs, MPI_Datatype rd, MPI_Comm c
) {
  MPI_Request request;
  Status ret = MPI_Iallgatherv(sb, sn, sd, rb, rn, displs, rd, c, &request);
  if (ret) ret = co_await request;
  co_return ret;
}
template <typename ST, typename RT>
auto allgatherv(
  const ST* sb, int sn, RT* rb, const int* rn, const int* displs, MPI_Comm c
) {
  return allgatherv(
    sb, datatype_count(sb, sn), datatype(sb), rb, rn, displs,
    datatype(rb), c
  );
}

inline MPITask probe(int src, int tag, MPI_Comm comm) {
  int found;
  Status ret;
  do {
    int found;
    ret = MPI_Iprobe(src, tag, comm, &found, ret);
    if (found || !ret) co_return ret;
    co_await std::suspend_always{};
  } while (true);
}

} // namespace fenix::mpixx

#endif // FENIX_MPIXX_TASKS_HPP
