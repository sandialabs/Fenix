#include "fenix/mpixx/tools.hpp"

#include "fenix/mpixx/util.hpp"

namespace fenix::mpixx {

Tools::~Tools() { finalize(); }

void Tools::init(int requested_thread_support) {
  if (initialized_) return;
  int ret = MPI_T_init_thread(requested_thread_support, &thread_support_);
  if (ret == MPI_SUCCESS) initialized_ = true;
}

void Tools::finalize() {
  if (!initialized_) return;
  MPI_T_finalize();
  initialized_ = false;
}

Tools::Tools(const Tools& o) { *this = o; }
Tools::Tools(Tools&& o) { *this = std::move(o); }

Tools& Tools::operator=(const Tools& o) {
  finalize();
  if (o.initialized_) {
    init(o.thread_support_);
  }
  return *this;
}
Tools& Tools::operator=(Tools&& o) {
  finalize();
  initialized_    = o.initialized_;
  thread_support_ = o.thread_support_;
  o.initialized_  = false;
  return *this;
}

} // namespace fenix::mpixx
