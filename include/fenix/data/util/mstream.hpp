#ifndef FENIX_DATA_MSTREAM_HPP
#define FENIX_DATA_MSTREAM_HPP

#include <streambuf>
#include <iostream>
#include <memory>

#include "fenix/data/util/data_ref.hpp"

namespace fenix::data::util {
namespace detail {

class OMmapStreamBuf : public std::streambuf {
 public:
  using std::streambuf::int_type;
  using std::streambuf::off_type;
  using std::streambuf::pos_type;
  using Traits = std::char_traits<char>;

#ifdef FENIX_HAVE_MREMAP
  // Bytes of virtual address space claimed at a time
  static constexpr size_t target_claim_chunk_size = 1024 * 1024 * 1024; // 1GB
#else
  // If we can't mremap, claim more virtual address space at once, since we
  // can't grow
  static constexpr size_t target_claim_chunk_size =
    20 * 1024 * 1024 * 1024; // 20GB
#endif

  // Bytes of claimed address space made writable at a time
  static constexpr size_t target_write_chunk_size = 1024 * 1024; // 1KB

  OMmapStreamBuf();
  ~OMmapStreamBuf() override;

  size_t written_len();

  // If FENIX_HAVE_MREMAP, user responsible for calling munmap on released buf.
  // Otherwise, user responsible for calling free on released buf.
  char* release();

 protected:
  int_type overflow(int_type ch) override;
  pos_type seekpos(pos_type pos, std::ios_base::openmode which) override;
  pos_type seekoff(
    off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which
  ) override;

 private:
  void grow_len(size_t& len, size_t chunk, size_t target);

  char* mmap_address       = nullptr;
  size_t claim_len         = 0;
  size_t writable_len      = 0;
  size_t written_highwater = 0;

  size_t claim_chunk_size, write_chunk_size;
};

class MStreamBuf : public std::streambuf {
 public:
  using std::streambuf::int_type;
  using std::streambuf::off_type;
  using std::streambuf::pos_type;
  using Traits = std::char_traits<char>;

  MStreamBuf(char* buf, size_t len);
  ~MStreamBuf() = default;

 protected:
  int_type overflow(int_type ch) override;
  pos_type seekpos(pos_type pos, std::ios_base::openmode which) override;
  pos_type seekoff(
    off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which
  ) override;

 private:
  char* buf  = nullptr;
  size_t len = 0;
};

} //namespace detail

class MStream : public std::iostream {
 public:
  // Dynamically sized MStream, will need its streambuf's data released
  MStream() : std::iostream(nullptr) {
    buf = std::make_unique<detail::OMmapStreamBuf>();
    rdbuf(buf.get());
  }

  // Statically sized MStream pointing to dr
  MStream(const DataRef& dr) : std::iostream(nullptr) {
    buf = std::make_unique<detail::MStreamBuf>(dr.data(), dr.size());
    rdbuf(buf.get());
  }

  MStream(MStream&& o) : std::iostream(nullptr) {
    o.rdbuf(nullptr);
    buf = std::move(o.buf);
    rdbuf(buf.get());
  }
  ~MStream() = default;

  std::streambuf* get_buf() { return buf.get(); }

 private:
  std::unique_ptr<std::streambuf> buf;
};

} //namespace fenix::data::util

#endif
