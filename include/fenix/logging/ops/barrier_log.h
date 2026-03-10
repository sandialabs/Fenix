#ifndef FENIX_LOGGING_OPS_BARRIER_LOG_H
#define FENIX_LOGGING_OPS_BARRIER_LOG_H
#include <cstring>
#include <istream>
#include <ostream>
#include "fenix/mpi_util.hpp"
#include "fenix/logging/op_log.h"

namespace fenix::logging {

class BarrierLog : public CollectiveLog {
 public:
  BarrierLog(MPI_Comm, int idx) : CollectiveLog(idx) {}
  BarrierLog(std::istream& i) : CollectiveLog(i) {}
  BarrierLog(BarrierLog&& o) { *this = std::move(o); }
  BarrierLog& operator=(BarrierLog&& o) {
    CollectiveLog::operator=(std::move(o));
    return *this;
  }

  ~BarrierLog() = default;

  void serialize_impl(std::ostream& s) const override {}

  std::string str() const override {
    return "Barrier " + std::to_string(m_idx);
  }

  int begin(MPI_Comm c) const override {
    // We need to convert to an Ibarrier to correctly match with the Ibarriers
    // during replay
    req_free();
    int ret = PMPI_Ibarrier(c, req());
    if (ret == MPI_SUCCESS) ret = PMPI_Wait(req(), MPI_STATUS_IGNORE);
    return ret;
  }

  void replay(MPI_Comm c) const override {
    req_free();
    int ret = PMPI_Ibarrier(c, req());
    fenix_assert(
      ret == MPI_SUCCESS, "Non-process MPI error during collective replay\n"
    );
  }
};

template <>
struct mpi_log<MPI_Barrier> {
  using type = BarrierLog;
};

} //namespace fenix::logging
#endif
