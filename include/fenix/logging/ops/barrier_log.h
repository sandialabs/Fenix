#ifndef FENIX_LOGGING_OPS_BARRIER_LOG_H
#define FENIX_LOGGING_OPS_BARRIER_LOG_H
#include <cstring>
#include <istream>
#include <ostream>
#include "fenix_util.hpp"
#include "fenix/logging/op_log.h"

namespace fenix::logging {

class BarrierLog : public CollectiveLog {
 public:
  BarrierLog(int idx) : CollectiveLog(idx) {}
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
    req_free();
    return PMPI_Ibarrier(c, req());
  }

  void write(BufferWrap buffer) const override { fenix_assert(!buffer); }
};

} //namespace fenix::logging
#endif
