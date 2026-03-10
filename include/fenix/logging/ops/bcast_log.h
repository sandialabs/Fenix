#ifndef FENIX_LOGGING_OPS_BCAST_LOG_H
#define FENIX_LOGGING_OPS_BCAST_LOG_H
#include <cstring>
#include <istream>
#include <ostream>
#include "fenix/mpi_util.hpp"
#include "fenix/logging/op_log.h"

namespace fenix::logging {

class BcastLog : public CollectiveLog {
 public:
  BcastLog(
    void* buffer, int count, MPI_Datatype type, int root_rank, MPI_Comm c,
    int idx
  )
    : CollectiveLog(idx), root(root_rank) {
    if (root == util::comm_rank(c)) {
      buf = MPIBuffer::copy(buffer, count, type);
    } else {
      buf = MPIBuffer::wrap(buffer, count, type);
    }
  }

  BcastLog(BcastLog&& o) { *this = std::move(o); }
  BcastLog& operator=(BcastLog&& o) {
    CollectiveLog::operator=(std::move(o));
    root = o.root;
    buf = std::move(o.buf);
    return *this;
  }

  ~BcastLog() = default;

  BcastLog(std::istream& i) : CollectiveLog(i) {
    serialize::read(i, root);
    serialize::read(i, buf);
  }
  void serialize_impl(std::ostream& s) const override {
    serialize::write(s, root);
    serialize::write(s, buf);
  }

  std::string str() const override {
    return "Bcast " + std::to_string(m_idx) +
           " (root = " + std::to_string(root) + ")";
  }

  int begin(MPI_Comm c) const override {
    req_free();
    int ret = PMPI_Ibcast(buf, buf, buf, root, c, req());
    if (ret == MPI_SUCCESS) ret = PMPI_Wait(req(), MPI_STATUS_IGNORE);
    // Release references to any user buffers if we get this far
    buf.release_user_buf();
    return ret;
  }

  void replay(MPI_Comm c) const override {
    req_free();
    int ret = PMPI_Ibcast(buf, buf, buf, root, c, req());
    fenix_assert(
      ret == MPI_SUCCESS, "Non-process MPI error during collective replay\n"
    );
  }

  int root;
  MPIBuffer buf;
};

template <>
struct mpi_log<MPI_Bcast> {
  using type = BcastLog;
};

} //namespace fenix::logging
#endif
