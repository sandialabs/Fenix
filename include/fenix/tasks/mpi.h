#ifndef FENIX_TASKS_MPI_H
#define FENIX_TASKS_MPI_H

#include <type_traits>
#include <utility>
#include <vector>
#include <mpi.h>
#include "task.h"

namespace fenix::util {
template <typename T>
MPI_Datatype datatype();

template <typename T>
MPI_Datatype datatype(T&& t) {
  return datatype<T>();
}

template <typename T>
constexpr int count(T&& t, int in_count);
}

namespace fenix::tasks::mpi {
// C++ type corresponding to MPI_Datatype index pairs
template <typename T>
struct Indexed {
  static_assert(std::is_trivially_copyable_v<T>);
  T value;
  int index;
};

using MPITask = Task<Status>;

template <typename T>
MPITask recv(T* b, int n, MPI_Datatype d, int r, int t, MPI_Comm c) {
  MPI_Request request;
  Status ret = MPI_Irecv(b, n, d, r, t, c, &request);
  if (ret) ret = co_await request;
  co_return ret;
}
template <typename T>
auto recv(T* b, int n, int r, int t, MPI_Comm c) {
  return recv(b, util::count(b, n), util::datatype(b), r, t, c);
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
  return send(b, util::count(b, n), util::datatype(b), r, t, c);
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
    sb, util::count(sb, sn), util::datatype(sb), sr, st,
    rb, util::count(rb, rn), util::datatype(rb), rr, rt, c
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
  return allreduce(sb, rb, util::count(sb, n), util::datatype(sb), o, c);
}
template <typename T>
auto allreduce(const T& sb, T& rb, MPI_Op o, MPI_Comm c) {
  return allreduce(&sb, rb, 1, o, c);
}
template <typename T, typename A>
auto allreduce(const std::vector<T, A>& sv, T& rb, MPI_Op o, MPI_Comm c) {
  return allreduce(&sv[0], rb, sv.size(), o, c);
}

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
  return reduce(sb, rb, util::count(sb, n), util::datatype(sb), o, r, c);
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
  return bcast(b, util::count(b, n), util::datatype(b), r, c);
}
template <typename T>
auto bcast(T& b, int r, MPI_Comm c) {
  return bcast(b, 1, r, c);
}
template <typename T, typename A>
auto bcast(std::vector<T, A>& v, int r, MPI_Comm c) {
  return bcast(&v[0], v.size(), r, c);
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
} // namespace fenix::tasks::mpi

namespace fenix::util {

#define MPI_TASK_TYPE(u, r, ...)                                               \
  if constexpr (std::is_same_v<u, __VA_ARGS__>) return r;

template <typename T>
MPI_Datatype datatype() {
  using namespace fenix::tasks::mpi;
  using U = std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>;
  static_assert(std::is_trivially_copyable_v<U>);
  // clang-format off
  MPI_TASK_TYPE(U, MPI_CHAR,            char);
  MPI_TASK_TYPE(U, MPI_FLOAT,           float);
  MPI_TASK_TYPE(U, MPI_DOUBLE,          double);
  MPI_TASK_TYPE(U, MPI_SHORT,           short);
  MPI_TASK_TYPE(U, MPI_UNSIGNED_SHORT,  unsigned short);
  MPI_TASK_TYPE(U, MPI_INT,             int);
  MPI_TASK_TYPE(U, MPI_UNSIGNED,        unsigned int);
  MPI_TASK_TYPE(U, MPI_LONG,            long);
  MPI_TASK_TYPE(U, MPI_UNSIGNED_LONG,   unsigned long);
  MPI_TASK_TYPE(U, MPI_LOGICAL,         bool);
  MPI_TASK_TYPE(U, MPI_FLOAT_INT,       Indexed<float>);
  MPI_TASK_TYPE(U, MPI_DOUBLE_INT,      Indexed<double>);
  MPI_TASK_TYPE(U, MPI_LONG_INT,        Indexed<long>);
  MPI_TASK_TYPE(U, MPI_2INT,            Indexed<int>);
  MPI_TASK_TYPE(U, MPI_SHORT_INT,       Indexed<short>);
  MPI_TASK_TYPE(U, MPI_LONG_DOUBLE_INT, Indexed<long double>);
  // clang-format on

  // Technically sketch to just make this MPI_BYTE, but only when heterogenenous
  // so we'll cross that bridge when we get there. Convenient for trivial custom
  // types for now
  return MPI_BYTE;
}

#undef MPI_TASK_TYPE

template <typename T>
constexpr int count(T&& t, int in_count) {
  if (datatype<T>() == MPI_BYTE) {
    return in_count * sizeof(std::remove_pointer_t<std::decay_t<T>>);
  }
  return in_count;
}
} // namespace fenix::util

#endif // FENIX_TASKS_MPI_H
