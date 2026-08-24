#ifndef FENIX_DATA_UTIL_DATA_REF_HPP
#define FENIX_DATA_UTIL_DATA_REF_HPP

#include "fenix/data/util/buffer.hpp"

#include <limits>

namespace fenix::data::util {

namespace detail {

template <bool IsConst>
class DataRefHolder {
 public:
  using DataT    = std::conditional_t<IsConst, const char, char>;
  using DataBufT = std::conditional_t<IsConst, const DataBuffer, DataBuffer>;

  DataRefHolder();
  DataRefHolder(DataT* b, size_t len) : raw_buf(b), raw_buf_size(len) {}
  DataRefHolder(DataBufT& db) : data_buf(&db) {}

  template <bool OtherConst>
  DataRefHolder(const DataRefHolder<OtherConst>& o) {
    *this = o;
  }
  template <bool OtherConst>
  DataRefHolder<IsConst>& operator=(const DataRefHolder<OtherConst>& o) {
    static_assert(IsConst || !OtherConst);
    raw_buf      = o.raw_buf;
    raw_buf_size = o.raw_buf_size;
    data_buf     = o.data_buf;
    return *this;
  }

  DataT* data() const { return data_buf ? data_buf->data() : raw_buf; }

  size_t size() const { return data_buf ? data_buf->size() : raw_buf_size; }

 protected:
  friend DataRefHolder<!IsConst>;

  DataBufT* data_buf = nullptr;

  DataT* raw_buf      = nullptr;
  size_t raw_buf_size = 0;
};

} //namespace detail

class DataRef {
 public:
  DataRef(char* b, size_t len) : ref(b, len) {}
  DataRef(char* b) : ref(b, std::numeric_limits<size_t>::max()) {}
  DataRef() : DataRef(nullptr) {}

  DataRef(DataBuffer& db) : ref(db) {}

  template <typename T, typename... U>
  DataRef(std::vector<T, U...>& v)
    : DataRef((char*)v.data(), v.size() * sizeof(T)) {}

  DataRef(const DataRef& dr) : ref(dr.ref) {}
  DataRef& operator=(const DataRef& dr) {
    ref = dr.ref;
    return *this;
  }

  char* data() const { return ref.data(); }
  size_t size() const { return ref.size(); }

  DataRef bounded(size_t max_size) {
    if (max_size < size()) return DataRef(data(), max_size);
    else return *this;
  }

  bool is_bounded() const {
    return size() != std::numeric_limits<size_t>::max();
  }

 protected:
  friend class ConstDataRef;

  detail::DataRefHolder<false> ref;
};

class ConstDataRef {
 public:
  ConstDataRef(const char* b, size_t len) : ref(b, len) {}
  ConstDataRef(const char* b) : ref(b, std::numeric_limits<size_t>::max()) {}
  ConstDataRef() : ConstDataRef(nullptr) {}

  ConstDataRef(const DataBuffer& db) : ref(db) {}

  template <typename T, typename... U>
  ConstDataRef(const std::vector<T, U...>& v)
    : ref(v.data(), v.size() * sizeof(T)) {}

  ConstDataRef(const DataRef& dr) : ref(dr.ref) {}
  ConstDataRef& operator=(const DataRef& dr) {
    ref = dr.ref;
    return *this;
  }

  ConstDataRef(const ConstDataRef& dr) : ref(dr.ref) {};
  ConstDataRef& operator=(const ConstDataRef& dr) {
    ref = dr.ref;
    return *this;
  }

  const char* data() const { return ref.data(); }
  size_t size() const { return ref.size(); }

  ConstDataRef bounded(size_t max_size) const {
    if (max_size < size()) return ConstDataRef(data(), max_size);
    else return *this;
  }

  bool is_bounded() const {
    return size() != std::numeric_limits<size_t>::max();
  }

 private:
  detail::DataRefHolder<true> ref;
};

} //namespace fenix::data::util

#endif
