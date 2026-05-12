#ifndef FENIX_TASKS_REQUEST_HPP
#define FENIX_TASKS_REQUEST_HPP

#include <coroutine>
#include <exception>
#include <cassert>

#include <mpi.h>

#include "subtask.hpp"
#include "fenix/mpi_util.hpp"

namespace fenix::tasks {

using util::Status;

// Note that this 'takes ownership' of the request - if RequestBase is destroyed
// before completing, it frees the MPI_Request to ensure proper cleanup if a
// Task is destroyed before completing.
class Request {
 public:
  Request(MPI_Request* r) : request(r) {};

  Request()               = default;
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
      ret      = MPI_Wait(request, ret);
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
    request  = r.request;
    ret      = r.ret;
    complete = r.complete;
    return *this;
  }

  MPI_Request* request = nullptr;
  Status ret;
  int complete = false;
};

} // namespace fenix::tasks

#endif //FENIX_TASKS_REQUEST_HPP
