#ifndef FENIX_LOGGING_OPS_ALLREDUCE_LOG_H
#define FENIX_LOGGING_OPS_ALLREDUCE_LOG_H
#include <cstring>
#include <istream>
#include <ostream>
#include "fenix/mpi_util.hpp"
#include "fenix/logging/op_log.h"

namespace fenix::logging {

class AllreduceLog : public CollectiveLog {
 public:
  AllreduceLog(
    const void* send, void* recv, int count, MPI_Datatype type, MPI_Op o,
    MPI_Comm c, int idx
  )
    : CollectiveLog(idx), op(o),
      sbuf(MPIBuffer::copy(send == MPI_IN_PLACE ? recv : send, count, type)),
      rbuf(MPIBuffer::wrap(recv, count, type)) {}

  AllreduceLog(AllreduceLog&& o) { *this = std::move(o); }
  AllreduceLog& operator=(AllreduceLog&& o) {
    CollectiveLog::operator=(std::move(o));
    op = o.op;
    sbuf = std::move(o.sbuf);
    rbuf = std::move(o.rbuf);
    return *this;
  }

  ~AllreduceLog() = default;

  AllreduceLog(std::istream& i) : CollectiveLog(i) {
    serialize::read(i, op);
    serialize::read(i, sbuf);
    rbuf = MPIBuffer::create(sbuf, sbuf);
  }
  void serialize_impl(std::ostream& s) const override {
    serialize::write(s, op);
    serialize::write(s, sbuf);
  }

  std::string str() const override {
    return "Allreduce " + std::to_string(m_idx);
  }

  int begin(MPI_Comm c) const override {
    req_free();
    int ret = PMPI_Iallreduce(sbuf, rbuf, sbuf, sbuf, op, c, req());
    if (ret == MPI_SUCCESS) ret = PMPI_Wait(req(), MPI_STATUS_IGNORE);
    // Release references to any user buffers if we get this far
    rbuf.release_user_buf();
    return ret;
  }

  void replay(MPI_Comm c) const override {
    req_free();
    int ret = PMPI_Iallreduce(sbuf, rbuf, sbuf, sbuf, op, c, req());
    fenix_assert(
      ret == MPI_SUCCESS, "Non-process MPI error during collective replay\n"
    );
  }

  MPI_Op op;
  MPIBuffer sbuf;
  MPIBuffer rbuf;
};

template <>
struct mpi_log<MPI_Allreduce> {
  using type = AllreduceLog;
};

} //namespace fenix::logging
#endif
