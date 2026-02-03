#ifndef MSG_LOG_H
#define MSG_LOG_H
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

class MsgLog {
 public:
  virtual ~MsgLog() { req_free(); }

  auto operator<=>(const MsgLog& o) const { return m_idx <=> o.m_idx; }
  auto operator<=>(const int& i) const { return m_idx <=> i; }
  auto operator==(const MsgLog& o) const { return m_idx == o.m_idx; }
  auto operator==(const int& i) const { return m_idx == i; }

  void serialize(std::ostream& o) const {
    serialize::write(o, m_idx);
    this->serialize_impl(o);
  };
  virtual void serialize_impl(std::ostream& o) const = 0;

  MPI_Request* req() const { return m_req; }
  int idx() const { return m_idx; }

  MsgLog(const MsgLog&) = delete;
  MsgLog& operator=(const MsgLog&) = delete;

 protected:
  MsgLog(int i) : m_idx(i) {}
  MsgLog(std::istream& i) : MsgLog(serialize::read<int>(i)) {}

  MsgLog(MsgLog&& o) : MsgLog(o.m_idx) {}
  MsgLog& operator=(MsgLog&& o) {
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

class SendLog : public MsgLog {
 public:
  SendLog(const void* b, int n, MPI_Datatype d, int t, int idx)
    : MsgLog(idx), datatype(d), tag(t) {
    data.resize(n * util::type_size(datatype));
    std::memcpy(data.data(), b, data.size());
  }
  ~SendLog() = default;

  SendLog(SendLog&& o) : MsgLog(std::move(o)) { *this = std::move(o); }
  SendLog& operator=(SendLog&& o) {
    MsgLog::operator=(std::move(o));
    data = std::move(o.data);
    datatype = o.datatype;
    tag = o.tag;
    return *this;
  }

  SendLog(std::istream& i) : MsgLog(i) {
    serialize::read(i, data);
    serialize::read(i, datatype);
    serialize::read(i, tag);
  }
  void serialize_impl(std::ostream& s) const override {
    serialize::write(s, data);
    serialize::write(s, datatype);
    serialize::write(s, tag);
  }

  int isend(int dst, MPI_Comm c) const {
    req_free();
    return PMPI_Isend(
      data.data(), data.size() / util::type_size(datatype), datatype, dst, tag,
      c, req()
    );
  }

  std::string str() const {
    return "Send " + std::to_string(m_idx) + " (tag " + std::to_string(tag) +
           ")";
  }

 private:
  mutable std::vector<char> data;
  mutable MPI_Datatype datatype;
  int tag;
};

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
    assert(*this);
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
