#ifndef FENIX_TASKS_REQUEST_H
#define FENIX_TASKS_REQUEST_H

#include <coroutine>
#include <exception>
#include <cassert>

#include <mpi.h>

#include "subtask.h"

namespace fenix::tasks {

class Status {
 public:
  int return_value;
  MPI_Status status;

  Status() = default;
  Status(int r) : return_value(r) {}
  auto operator=(int r) {
    return_value = r;
    return *this;
  }

  operator bool() const { return return_value == MPI_SUCCESS; }

  operator int() const { return return_value; }
  bool operator==(int r) const { return return_value == r; }

  operator MPI_Status() const { return status; }
  operator MPI_Status*() { return &status; }

  // to support structured unbinding
  template <size_t I>
  auto&& get() && {
    if constexpr (I == 0) return std::move(return_value);
    if constexpr (I == 1) return std::move(status);
  }
};

// Note that this 'takes ownership' of the request - if RequestBase is destroyed
// before completing, it frees the MPI_Request to ensure proper cleanup if a
// Task is destroyed before completing.
class Request {
 public:
  Request(MPI_Request* r) : request(r) {};

  Request() = default;
  Request(const Request&) = delete;
  Request(Request&& r) { *this = std::move(r); }
  Request& operator=(MPI_Request* r) { return *this = Request(r); }
  Request& operator=(Request&& r) {
    mpi_free();
    *this = r;
    Request tmp{};
    r = tmp;
    return *this;
  }

  // TODO: this turned out buggy, figure out why
  ~Request() { /*mpi_free();*/ }

  bool operator==(const MPI_Request* r) const { return r == request; }

  bool is_complete() {
    return complete || request == nullptr || *request == MPI_REQUEST_NULL;
  }

  bool test() {
    if (!is_complete()) {
      ret = MPI_Test(request, &complete, ret);
      if (ret != MPI_SUCCESS) complete = true;
    }
    return is_complete();
  }
  void wait() {
    if (!is_complete()) {
      ret = MPI_Wait(request, ret);
      complete = true;
    }
  }
  void mpi_free() {
    if (!is_complete()) MPI_Request_free(request);
  }

  bool done() { return test(); }
  void resume() { test(); }
  auto result() {
    assert(done());
    return ret;
  }

 private:
  Request& operator=(const Request& r) {
    request = r.request;
    ret = r.ret;
    complete = r.complete;
    return *this;
  }

  MPI_Request* request = nullptr;
  Status ret;
  int complete = false;
};

} // namespace fenix::tasks

// Supporting structured unbinding for Status
namespace std {
template <>
struct tuple_size<fenix::tasks::Status> : std::integral_constant<size_t, 2> {};

template <>
struct tuple_element<0, fenix::tasks::Status> {
  using type = int;
};
template <>
struct tuple_element<1, fenix::tasks::Status> {
  using type = MPI_Status;
};
}

#endif //FENIX_TASKS_REQUEST_H
