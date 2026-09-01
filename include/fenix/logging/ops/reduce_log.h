#ifndef FENIX_LOGGING_OPS_REDUCE_LOG_H
#define FENIX_LOGGING_OPS_REDUCE_LOG_H
#include <cstring>
#include <istream>
#include <ostream>
#include "fenix/mpixx/util.hpp"
#include "fenix/logging/op_log.h"

namespace fenix::logging {

class ReduceLog : public CollectiveLog {
 public:
  ReduceLog(
    const void* send, void* recv, int count, MPI_Datatype type, MPI_Op o,
    int root_rank, MPI_Comm c, int idx
  )
    : CollectiveLog(idx), root(root_rank), op(o),
      sbuf(MPIBuffer::copy(send == MPI_IN_PLACE ? recv : send, count, type)) {
    if (root == mpixx::comm_rank(c)) {
      rbuf = MPIBuffer::wrap(recv, count, type);
    }
  }

  ReduceLog(ReduceLog&& o) { *this = std::move(o); }
  ReduceLog& operator=(ReduceLog&& o) {
    CollectiveLog::operator=(std::move(o));
    root = o.root;
    op   = o.op;
    sbuf = std::move(o.sbuf);
    rbuf = std::move(o.rbuf);
    return *this;
  }

  ~ReduceLog() = default;

  ReduceLog(std::istream& i) : CollectiveLog(i) {
    serialize::read(i, root);
    serialize::read(i, op);
    serialize::read(i, sbuf);
    rbuf = MPIBuffer::create(sbuf, sbuf);
  }
  void serialize_impl(std::ostream& s) const override {
    serialize::write(s, root);
    serialize::write(s, op);
    serialize::write(s, sbuf);
  }

  std::string str() const override {
    return "Reduce " + std::to_string(m_idx) +
           " (root = " + std::to_string(root) + ")";
  }

  int begin(MPI_Comm c) const override {
    req_free();
    void* recv = root == mpixx::comm_rank(c) ? rbuf.buf() : nullptr;
    int ret    = PMPI_Ireduce(sbuf, recv, sbuf, sbuf, op, root, c, req());
    if (ret == MPI_SUCCESS) ret = PMPI_Wait(req(), MPI_STATUS_IGNORE);
    // Release references to any user buffers if we get this far
    rbuf.release_user_buf();
    return ret;
  }

  void replay(MPI_Comm c) const override {
    req_free();
    void* recv = root == mpixx::comm_rank(c) ? rbuf.buf() : nullptr;
    int ret    = PMPI_Ireduce(sbuf, recv, sbuf, sbuf, op, root, c, req());
    fenix_assert(
      ret == MPI_SUCCESS, "Non-process MPI error during collective replay\n"
    );
  }

  int root;
  MPI_Op op;
  MPIBuffer sbuf;
  MPIBuffer rbuf;
};

template <>
struct mpi_log<MPI_Reduce> {
  using type = ReduceLog;
};

} //namespace fenix::logging
#endif
