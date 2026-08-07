#include "fenix/data/util/mfile.hpp"
#include "fenix/data/util/data_ref.hpp"
#include "fenix_opt.hpp"

#include <stdio.h>

namespace fenix::data::util {

void MFile::open(const ConstDataRef& r, const char m) {
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

void MFile::close() {
  fenix_assert(!is_dynamic);
  close_file();
}

char* MFile::close_dynamic() {
  fenix_assert(is_dynamic);
  return close_file();
}

MFile::~MFile() {
  if (file_ptr) {
    fenix_assert(!is_dynamic);
    close_file();
  }
}

char* MFile::close_file() {
  fenix_assert(file_ptr);
  fclose(file_ptr);
  char* ret  = file_mem;
  file_ptr   = nullptr;
  is_dynamic = false;
  file_mem   = nullptr;
  file_size  = 0;
  return ret;
}

void MFile::open_dynamic() {
  fenix_assert(!file_ptr);
  file_size  = 0;
  is_dynamic = true;
  file_ptr   = open_memstream(&file_mem, &file_size);
  fenix_assert(file_ptr);
}

size_t MFile::size() {
  if (is_dynamic) fflush(file_ptr);
  return file_size;
}

} //namespace fenix::data::util
