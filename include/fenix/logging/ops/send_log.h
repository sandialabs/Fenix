#ifndef FENIX_LOGGING_OPS_SEND_LOG_H
#define FENIX_LOGGING_OPS_SEND_LOG_H
#include <cstring>
#include <istream>
#include <ostream>
#include "fenix/logging/serialize.h"
#include "fenix/logging/op_log.h"

namespace fenix::logging {

class SendLog : public OpLog {
 public:
  SendLog(const void* b, int n, MPI_Datatype d, int t, int idx)
    : OpLog(idx), buf(MPIBuffer::copy(b, n, d)), tag(t) {}
  ~SendLog() = default;

  SendLog(SendLog&& o) { *this = std::move(o); }
  SendLog& operator=(SendLog&& o) {
    OpLog::operator=(std::move(o));
    buf = std::move(o.buf);
    tag = o.tag;
    return *this;
  }

  SendLog(std::istream& i) : OpLog(i) {
    serialize::read(i, buf);
    serialize::read(i, tag);
  }
  void serialize_impl(std::ostream& s) const override {
    serialize::write(s, buf);
    serialize::write(s, tag);
  }

  int isend(int dst, MPI_Comm c) const {
    req_free();
    return PMPI_Isend(buf, buf, buf, dst, tag, c, req());
  }

  std::string str() const override {
    return "Send " + std::to_string(m_idx) + " (tag " + std::to_string(tag) +
      ")";
  }

 private:
  MPIBuffer buf;
  int tag;
};

} //namespace fenix::logging
#endif
