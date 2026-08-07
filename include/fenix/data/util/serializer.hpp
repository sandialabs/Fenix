#ifndef FENIX_DATA_UTIL_SERIALIZER_HPP
#define FENIX_DATA_UTIL_SERIALIZER_HPP

#include <memory>
#include <optional>

#include "fenix.hpp"
#include "fenix/data/util/data_ref.hpp"
#include "fenix/data/util/mfile.hpp"
#include "fenix/data/util/mstream.hpp"

namespace fenix::data::util {

class Serializer {
 public:
  Serializer(
    DataBuffer& stage_buf, std::optional<SerializeFunc>& f,
    const DataRef& user_buf, int direction, int element_size
  );

  void serialize_elements(int first, int last) const;

  size_t size() const;

  size_t max_element() const;

  ~Serializer();

  bool has_file() { return file != nullptr; }
  FILE* get_file();

  bool has_stream() { return !!strm; }
  std::iostream* get_stream();

  int get_dir() const { return dir; }

  Serializer(Serializer&& o);

 private:
  bool use_file_func = false;
  SerializeFileFunc file_func;
  mutable std::unique_ptr<MFile> file;

  bool use_strm_func = false;
  SerializeStreamFunc strm_func;
  mutable std::optional<MStream> strm;

  DataBuffer& stage;
  DataRef user;
  int dir;
  bool dynamic;
  int elm_size;
};

} //namespace fenix::data::util

#endif
