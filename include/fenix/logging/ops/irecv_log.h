#ifndef FENIX_LOGGING_OPS_IRECV_LOG_H
#define FENIX_LOGGING_OPS_IRECV_LOG_H

#include <cstring>
#include <string>
#include <mpi.h>
#include "fenix_util.hpp"

namespace fenix::logging {

struct IrecvLog {
  IrecvLog() = default;
  IrecvLog(void* b, int c, MPI_Datatype d, int t, MPI_Request* r)
    : buf(b), count(c), datatype(d), tag(t), request(r) {}
  IrecvLog& operator=(const IrecvLog& o) {
    buf = o.buf;
    count = o.count;
    datatype = o.datatype;
    tag = o.tag;
    request = o.request;
    return *this;
  }

  void* buf = nullptr;
  int count = -1;
  MPI_Datatype datatype;
  int tag = -1;
  MPI_Request* request = nullptr;

  int irecv(int src, MPI_Comm comm) {
    fenix_assert(*this);
    return PMPI_Irecv(buf, count, datatype, src, tag, comm, request);
  }
  bool operator==(MPI_Request* const& r) const { return request == r; }
  void reset() { *this = IrecvLog(); }
  operator bool() const { return request != nullptr; }
  std::string str() const {
    return "Recv 0x" + std::to_string((uintptr_t)request) + " (tag " +
           std::to_string(tag) + ")";
  }
};

} //namespace fenix::logging
#endif
