#ifndef FENIX_DATA_MFILE_HPP
#define FENIX_DATA_MFILE_HPP

#include <streambuf>
#include <iostream>
#include <sys/mman.h>
#include <errno.h>
#include <cstring>

#include "fenix_opt.hpp"
#include "fenix_data_subset.hpp"
#include "fenix/data/util/data_ref.hpp"

namespace fenix::data::util {

class MFile {
 public:
  MFile()        = default;
  MFile(MFile&&) = delete;

  void open(const ConstDataRef& r) { open(r, 'r'); }

  void open(const DataRef& r) { open(r, 'w'); }

  void open_dynamic() {
    fenix_assert(!file_ptr);
    file_size  = 0;
    is_dynamic = true;
    file_ptr   = open_memstream(&file_mem, &file_size);
    fenix_assert(file_ptr);
  }

  bool dynamic() { return is_dynamic; }
  FILE* fp() { return file_ptr; }
  size_t size() {
    fflush(file_ptr);
    return file_size;
  }

  void close() {
    fenix_assert(!is_dynamic);
    close_file();
  }

  char* close_dynamic() {
    fenix_assert(is_dynamic);
    return close_file();
  }

  ~MFile() {
    if (file_ptr) {
      fenix_assert(!is_dynamic);
      close_file();
    }
  }

 private:
  void open(const ConstDataRef& r, const char m) {
    fenix_assert(!file_ptr);
    file_size  = r.size();
    is_dynamic = false;

    char first_byte, last_byte;
    if (m == 'w' && r.size() > 0) {
      first_byte = r.data()[0];
      last_byte  = r.data()[r.size() - 1];
    }

    char* buf = const_cast<char*>(r.data());
    file_ptr  = fmemopen(buf, r.size(), &m);
    fenix_assert(file_ptr);

    if (m == 'w' && r.size() > 0) {
      fseek(file_ptr, r.size() - 1, SEEK_SET);
      fwrite(" ", 1, 1, file_ptr);
      buf[0]            = first_byte;
      buf[r.size() - 1] = last_byte;
    }

    // Don't buffer writes, just write straight to data buf
    setbuf(file_ptr, nullptr);
  }

  char* close_file() {
    fenix_assert(file_ptr);
    fclose(file_ptr);
    char* ret  = file_mem;
    file_ptr   = nullptr;
    is_dynamic = false;
    file_mem   = nullptr;
    file_size  = 0;
    return ret;
  }

  FILE* file_ptr   = nullptr;
  bool is_dynamic  = false;
  char* file_mem   = nullptr;
  size_t file_size = 0;
};

} //namespace fenix::data::util

#endif
