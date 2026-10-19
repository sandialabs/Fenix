#ifndef FENIX_MPIXX_TOOLS_HPP
#define FENIX_MPIXX_TOOLS_HPP

#include <mpi.h>

namespace fenix::mpixx {

class Tools {
 public:
  Tools() noexcept = default;
  Tools(int req_thread_support) { init(req_thread_support); }
  ~Tools();

  Tools(const Tools&);
  Tools& operator=(const Tools&);
  Tools(Tools&&);
  Tools& operator=(Tools&&);

  void init(int requested_thread_support = MPI_THREAD_SINGLE);
  void finalize();

  bool is_initialized() const noexcept { return initialized_; }
  explicit operator bool() const noexcept { return is_initialized(); }

  int provided_thread_support() const noexcept { return thread_support_; }

 private:
  bool initialized_   = false;
  int thread_support_ = MPI_THREAD_SINGLE;
};

} // namespace fenix::mpixx

#endif // FENIX_MPIXX_TOOLS_HPP
