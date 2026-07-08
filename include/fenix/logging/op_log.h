#ifndef FENIX_LOGGING_OP_LOG_H
#define FENIX_LOGGING_OP_LOG_H
#include <cstring>
#include <istream>
#include <ostream>
#include "fenix_opt.hpp"
#include "fenix/mpi_util.hpp"
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

template <auto MPIFunction>
struct mpi_log;

template <auto MPIFunction>
using mpi_log_t = typename mpi_log<MPIFunction>::type;

class CollectiveLog : public OpLog {
 public:
  using OpLog::OpLog;

  // Launch collective for the (locally) first time
  virtual int begin(MPI_Comm c) const = 0;

  // Asynchronously replay collective
  virtual void replay(MPI_Comm c) const = 0;

 protected:
  CollectiveLog& operator=(CollectiveLog&& o) {
    OpLog::operator=(std::move(o));
    return *this;
  }
};

class MPIBuffer {
 public:
  MPIBuffer() = default;

  // No copying, move-only
  MPIBuffer(const MPIBuffer&) = delete;
  MPIBuffer& operator=(const MPIBuffer& o) = delete;
  MPIBuffer(MPIBuffer&& o) { *this = std::move(o); }
  MPIBuffer& operator=(MPIBuffer&& o) {
    m_count = o.m_count;
    o.m_count = 0;
    m_type = o.m_type;
    o.m_type = MPI_DATATYPE_NULL;
    internal_buf = std::move(o.internal_buf);
    user_buf = o.user_buf;
    o.user_buf = nullptr;
    return *this;
  }

  // Wrap a user's buffer
  static MPIBuffer wrap(void* user_buffer, int count, MPI_Datatype type) {
    return MPIBuffer(user_buffer, count, type);
  }
  // Create a buffer with uninitialized data
  static MPIBuffer create(int count, MPI_Datatype type) {
    return MPIBuffer(count, type);
  }
  // Create a buffer and copy from the user's buffer
  static MPIBuffer copy(const void* user_buffer, int count, MPI_Datatype type) {
    return MPIBuffer(count, type, static_cast<const char*>(user_buffer));
  }

  // Check if this object was default initialized or not
  operator bool() const { return m_type != MPI_DATATYPE_NULL; }

  void copy_to(void* out) {
    std::memcpy(out, buf(), m_count * util::type_size(m_type));
  }
  void copy_from(void* in) {
    std::memcpy(buf(), in, m_count * util::type_size(m_type));
  }

  // Release pointer to user buffer, to avoid use-after-free
  void release_user_buf() const { user_buf = nullptr; }

  void serialize(std::ostream& o) const {
    serialize::write(o, m_type);
    serialize::write(o, m_count);

    int size = garbage_data ? 0 : internal_buf.size();
    serialize::write<int>(o, size);
    if (size > 0) serialize::write(o, internal_buf.data(), size);
  }
  MPIBuffer(std::istream& i) {
    serialize::read(i, m_type);
    serialize::read(i, m_count);

    int size = serialize::read<int>(i);
    if (size > 0) {
      internal_buf.resize(size);
      serialize::read(i, &internal_buf[0], size);
    }
  }

  void* buf() const {
    if (user_buf) {
      // This should always be either a user buf wrapper OR hold its own buf
      fenix_assert(internal_buf.empty());
      return user_buf;
    }
    if (internal_buf.empty()) {
      // We're allocating the buffer to store unused output data into during
      // replay, so track that this should not be serialized
      garbage_data = true;
      internal_buf.resize(m_count * util::type_size(m_type));
    }
    fenix_assert(internal_buf.size() == m_count * util::type_size(m_type));
    return internal_buf.data();
  };
  int count() const { return m_count; }
  MPI_Datatype type() const { return m_type; }

  operator void*() const { return buf(); }
  operator int() const { return m_count; }
  operator MPI_Datatype() const { return m_type; }

 private:
  // Wrapping constructor
  MPIBuffer(void* user_buffer, int count, MPI_Datatype type)
    : user_buf(user_buffer), m_count(count), m_type(type) {};
  // Copying constructor
  MPIBuffer(int count, MPI_Datatype type, const char* b)
    : internal_buf(b, b + count * util::type_size(type)), m_count(count),
      m_type(type) {};
  // Creating constructor
  MPIBuffer(int count, MPI_Datatype type) : m_count(count), m_type(type) {};

  mutable void* user_buf = nullptr;
  mutable std::vector<char> internal_buf;
  // Sometimes internal_buf is only allocated as a receive buffer for replayed
  // operations. In that case, we don't serialize the data
  mutable bool garbage_data = false;

  int m_count = 0;
  MPI_Datatype m_type = MPI_DATATYPE_NULL;
};

} //namespace fenix::logging
#endif
