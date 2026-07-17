#ifndef FENIX_DATA_MSTREAM_HPP
#define FENIX_DATA_MSTREAM_HPP

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <streambuf>
#include <iostream>
#include <sys/mman.h>
#include <errno.h>
#include <cstring>

#include "fenix_opt.hpp"

namespace fenix::data {
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

  OMmapStreamBuf() : std::streambuf() {
    // Operate in page sized chunks, for portability
    size_t page_size = sysconf(_SC_PAGESIZE);
    claim_chunk_size = (target_claim_chunk_size / page_size) * page_size;
    if (claim_chunk_size < target_claim_chunk_size * 0.75)
      claim_chunk_size += page_size;
    write_chunk_size = (target_write_chunk_size / page_size) * page_size;
    if (write_chunk_size < target_write_chunk_size * 0.75)
      write_chunk_size += page_size;
    fenix_assert(write_chunk_size <= claim_chunk_size);

    claim_len = claim_chunk_size;
    mmap_address =
      (char*)mmap(nullptr, claim_len, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (mmap_address == MAP_FAILED) {
      fatal_print("Could not mmap an address, errno %d", errno);
    }

    writable_len = write_chunk_size;
    int err      = mprotect(mmap_address, writable_len, PROT_WRITE);
    if (err != 0) {
      fatal_print("Could not mprotect the allocation, errno %d", errno);
    }

    setp(mmap_address, mmap_address + writable_len);
  }

  ~OMmapStreamBuf() override {
    if (mmap_address) munmap(mmap_address, claim_len);
  }

  size_t written_len() {
    size_t ret = pptr() - mmap_address;
    if (ret < written_highwater) ret = written_highwater;
    return ret;
  }

  // buf should no longer be used after calling release()
  // If FENIX_HAVE_MREMAP, user responsible for calling munmap on released buf.
  // Otherwise, user responsible for calling free on released buf.
  char* release() {
    char* ret = nullptr;

#ifdef FENIX_HAVE_MREMAP
    ret = (char*
    )mremap(mmap_address, claim_len, written_len(), MREMAP_MAYMOVE, nullptr);
    if (ret == MAP_FAILED) fatal_print("Unexpected mremap failure");
#else
    // We can't release the big virtual address reservation without munmaping
    char* ret = (char*)malloc(written_len());
    memcpy(ret, mmap_address, written_len());
    munmap(mmap_address, claim_len);
#endif

    mmap_address = nullptr;
    claim_len = writable_len = written_highwater = 0;
    setp(nullptr, nullptr);
    return ret;
  }

 protected:
  int_type overflow(int_type ch) override {
    setp(pptr(), mmap_address + writable_len);
    // No need to write ch if it's eof, we're simply done.
    if (Traits::eq_int_type(ch, Traits::eof())) return Traits::not_eof(ch);

    size_t needed_len = pptr() - mmap_address + 1;
    if (claim_len < needed_len) {
#ifdef FENIX_HAVE_MREMAP
      size_t old_claim_len = claim_len;
      grow_len(claim_len, claim_chunk_size, needed_len);
      mmap_address = (char*
      )mremap(mmap_address, old_claim_len, claim_len, MREMAP_MAYMOVE, nullptr);

      if (mmap_address == MAP_FAILED) {
        fatal_print("Out of space for serialization");
      }
#else
      fatal_print("Out of space for serialization");
#endif
    }
    if (writable_len < needed_len) {
      grow_len(writable_len, write_chunk_size, needed_len);
      mprotect(mmap_address, writable_len, PROT_WRITE);
    }

    *pptr() = Traits::to_char_type(ch);
    setp(pptr() + 1, mmap_address + writable_len);

    return ch;
  }

  /*std::streamsize xsputn(const char* s, std::streamsize count) {
    size_t written = 0;
    while (written < count) {
      size_t avail = writable_len - (pptr() - mmap_address);
      if (avail > count - written) avail = count = written;

      if (avail) {
        memcpy(pptr(), s+written, count);
        written += avail;
      } else {
        overflow(s[written]);
        written++;
      }
    }
    return count;
  }*/

  pos_type seekpos(pos_type pos, std::ios_base::openmode which) {
    if (!(which & std::ios_base::out)) return 0;
    written_highwater = written_len();
    setp(mmap_address + pos, mmap_address + writable_len);
    return pos;
  }

  pos_type seekoff(
    off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which
  ) {
    if (!(which & std::ios_base::out)) return 0;
    written_highwater = written_len();

    char* new_ptr = nullptr;
    if (dir == std::ios_base::beg) new_ptr = mmap_address + off;
    else if (dir == std::ios_base::cur) new_ptr = pptr() + off;
    else new_ptr = mmap_address + written_len() + off;

    setp(new_ptr, mmap_address + writable_len);
    return new_ptr - mmap_address;
  }

 private:
  void grow_len(size_t& len, size_t chunk, size_t target) {
    if (len >= target) return;
    len = (target / chunk) * chunk;
    if (target % chunk != 0) len += chunk;
    fenix_assert(len >= target);
  }

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

  MStreamBuf(char* b, size_t l) : std::streambuf() {
    buf = b;
    len = l;
    setp(buf, buf + len);
    setg(buf, buf, buf + len);
  }

  ~MStreamBuf() override = default;

 protected:
  int_type overflow(int_type ch) override {
    // This class is not designed to dynamically allocate, any accesses outside
    // of the pre-sized region are erroneous.
    fatal_print("MStreamBuf overflow");
  }

  pos_type seekpos(pos_type pos, std::ios_base::openmode which) {
    if (which & std::ios_base::out) setp(buf + pos, buf + len);
    if (which & std::ios_base::in) setg(buf, buf + pos, buf + len);
    return pos;
  }

  pos_type seekoff(
    off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which
  ) {
    if (dir == std::ios_base::cur) {
      if (which & std::ios_base::out) setp(pptr() + off, buf + len);
      if (which == std::ios_base::out) return pptr() - buf;
      setg(buf, gptr() + off, buf + len);
      return gptr() - buf;
    } else {
      size_t pos = dir == std::ios_base::beg ? off : len + off;
      return seekpos(pos, which);
    }
  }

 private:
  char* buf  = nullptr;
  size_t len = 0;
};

} //namespace detail

class MStream : public std::iostream {
 public:
  MStream() : std::iostream(nullptr) {
    buf = std::make_unique<detail::OMmapStreamBuf>();
    rdbuf(buf.get());
  }
  MStream(char* b, size_t l) : std::iostream(nullptr) {
    buf = std::make_unique<detail::MStreamBuf>(b, l);
    rdbuf(buf.get());
  }
  ~MStream() = default;

  std::streambuf* get_buf() { return buf.get(); }

 private:
  std::unique_ptr<std::streambuf> buf;
};

} //namespace fenix::data

#endif
