#ifndef FENIX_LOGGING_OP_LOG_H
#define FENIX_LOGGING_OP_LOG_H
#include <cstring>
#include <istream>
#include <ostream>
#include "fenix_util.hpp"
#include "fenix/logging/serialize.h"

namespace fenix::logging {

void record_mpi_type(MPI_Datatype d);
void record_mpi_op(MPI_Op o);

// idempotent
void init_mpi_records();

class OpLog {
 public:
  virtual ~OpLog() { req_free(); }

  auto operator<=>(const OpLog& o) const { return m_idx <=> o.m_idx; }
  auto operator<=>(const int& i) const { return m_idx <=> i; }
  auto operator==(const OpLog& o) const { return m_idx == o.m_idx; }
  auto operator==(const int& i) const { return m_idx == i; }

  void serialize(std::ostream& o) const {
    serialize::write(o, m_idx);
    this->serialize_impl(o);
  };

  MPI_Request* req() const { return m_req; }
  int idx() const { return m_idx; }

  OpLog(const OpLog&) = delete;
  OpLog& operator=(const OpLog&) = delete;

  virtual void serialize_impl(std::ostream& o) const = 0;
  virtual std::string str() const = 0;

 protected:
  OpLog() : m_idx(-1) {}
  OpLog(int i) : m_idx(i) {}
  OpLog(std::istream& i) : OpLog(serialize::read<int>(i)) {}

  OpLog& operator=(OpLog&& o) {
    m_idx = o.m_idx;
    return *this;
  }

  void req_set(MPI_Request* new_ptr) const {
    req_free();
    m_req = new_ptr;
  }
  void req_free() const {
    if (req_obj != MPI_REQUEST_NULL) {
      if (!util::mpi_finalized()) MPI_Request_free(&req_obj);
      req_obj = MPI_REQUEST_NULL;
    }
    m_req = &req_obj;
  }

  int m_idx;

 private:
  mutable MPI_Request req_obj = MPI_REQUEST_NULL;
  mutable MPI_Request* m_req = &req_obj;
};

class CollectiveLog : public OpLog {
 public:
  using OpLog::OpLog;

  // Asynchronously launch collective
  virtual int begin(MPI_Comm c) const = 0;

  // Write collective output to user buffer
  virtual void write(class BufferWrap buffer) const = 0;

 protected:
  CollectiveLog& operator=(CollectiveLog&& o) {
    OpLog::operator=(std::move(o));
    return *this;
  }
};

// Copy a user's buffer with type and size info (or make space for an output)
class BufferCopy {
 public:
  BufferCopy() = default;
  BufferCopy(int count, MPI_Datatype datatype)
    : type(datatype), buf(count * util::type_size(datatype)) {}
  BufferCopy(const void* user_buf, int count, MPI_Datatype datatype)
    : BufferCopy(count, datatype) {
    fenix_assert(user_buf != MPI_IN_PLACE);
    fenix_assert(user_buf || type == MPI_DATATYPE_NULL);
    std::memcpy(buf.data(), user_buf, buf.size());
  }

  // Can be (de)serialized
  BufferCopy(std::istream& i) {
    serialize::read(i, type);
    if (type != MPI_DATATYPE_NULL) serialize::read(i, buf);
  }
  void serialize(std::ostream& o) const {
    serialize::write(o, type);
    if (type != MPI_DATATYPE_NULL) serialize::write(o, buf);
  }

  // No copying, move-only
  BufferCopy(const BufferCopy&) = delete;
  BufferCopy& operator=(const BufferCopy& o) = delete;
  BufferCopy(BufferCopy&& o) { *this = std::move(o); }
  BufferCopy& operator=(BufferCopy&& o) {
    buf = std::move(o.buf);
    type = o.type;
    o.type = MPI_DATATYPE_NULL;
    return *this;
  }

  // Convert to types needed for MPI operations
  operator void*() const {
    if (type == MPI_DATATYPE_NULL) return nullptr;
    return buf.data();
  }
  operator int() const {
    if (type == MPI_DATATYPE_NULL) return 0;
    return buf.size() / util::type_size(type);
  }
  operator MPI_Datatype() const { return type; }

  MPI_Datatype type = MPI_DATATYPE_NULL;
  mutable std::vector<char> buf;
};

// A user's buffer wrapped with type and size info
class BufferWrap {
 public:
  BufferWrap(void* m_buf, int m_count, MPI_Datatype m_type)
    : buf(m_buf), count(m_count), type(m_type) {
    fenix_assert(buf || type == MPI_DATATYPE_NULL);
  };

  // Copy from a BufferCopy
  BufferWrap& operator=(const BufferCopy& o) {
    fenix_assert(buf || type == MPI_DATATYPE_NULL);
    fenix_assert(type == o.type);
    fenix_assert(count == o.buf.size() / util::type_size(type));
    std::memcpy(buf, o.buf.data(), o.buf.size());
  }

  operator bool() const { return !buf || type == MPI_DATATYPE_NULL; }

  void* buf = nullptr;
  int count = 0;
  MPI_Datatype type = MPI_DATATYPE_NULL;
};

} //namespace fenix::logging
#endif
