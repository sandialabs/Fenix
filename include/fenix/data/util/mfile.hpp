#ifndef FENIX_DATA_MFILE_HPP
#define FENIX_DATA_MFILE_HPP

#include "fenix_opt.hpp"
#include "fenix/data/util/data_ref.hpp"

#include <stdio.h>

namespace fenix::data::util {

class MFile {
 public:
  MFile()        = default;
  MFile(MFile&&) = delete;

  void open(const ConstDataRef& r) { open(r, 'r'); }
  void open(const DataRef& r) { open(r, 'w'); }
  void open_dynamic();

  void close();
  char* close_dynamic();

  bool dynamic() { return is_dynamic; }
  FILE* fp() { return file_ptr; }
  size_t size();

  ~MFile();

 private:
  void open(const ConstDataRef& r, const char m);
  char* close_file();

  FILE* file_ptr   = nullptr;
  bool is_dynamic  = false;
  char* file_mem   = nullptr;
  size_t file_size = 0;
};

} //namespace fenix::data::util

#endif
