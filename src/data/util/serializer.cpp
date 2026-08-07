#include "fenix/data/util/serializer.hpp"
#include "fenix/data/util/mstream.hpp"
#include "fenix/data/util/mfile.hpp"
#include "fenix/data/util/data_ref.hpp"
#include "fenix_opt.hpp"

#include <cstring>

namespace fenix::data::util {

Serializer::Serializer(
  DataBuffer& stage_buf, std::optional<SerializeFunc>& f,
  const DataRef& user_buf, int direction, int element_size
) : stage(stage_buf), user(user_buf), dir(direction), elm_size(element_size) {
  dynamic = !user_buf.is_bounded() && dir == FENIX_SERIALIZE;
  if (dynamic) fenix_assert(f);
  else fenix_assert(size() % elm_size == 0);

  if (f) {
    if (std::holds_alternative<SerializeFileFunc>(*f)) {
      use_file_func = true;
      file_func     = std::get<SerializeFileFunc>(*f);
    } else if (std::holds_alternative<SerializeStreamFunc>(*f)) {
      use_strm_func = true;
      strm_func     = std::get<SerializeStreamFunc>(*f);
    } else {
      fatal_print("Unknown serialize function type");
    }
  }

  if (use_file_func) {
    file = std::make_unique<MFile>();
    if (dynamic) file->open_dynamic();
    else if (dir == FENIX_SERIALIZE) file->open(DataRef(stage));
    else file->open(ConstDataRef(stage));
  } else if (use_strm_func) {
    if (dynamic) strm.emplace();
    else strm.emplace(stage);
  }
};

void Serializer::serialize_elements(int first, int last) const {
  int count;
  if (dynamic) {
    fenix_assert(first == 0);
    count = FENIX_RESIZEABLE;
  } else {
    fenix_assert(first >= 0 && last <= max_element());
    count = last - first + 1;
  }
  size_t byte_offset = first * elm_size;

  if (use_file_func) {
    fenix_assert(file_func);
    fseek(file->fp(), byte_offset, SEEK_SET);
    file_func(file->fp(), dir, user.data(), first, count);
  } else if (use_strm_func) {
    fenix_assert(strm_func);
    strm->seekg(byte_offset);
    strm->seekp(byte_offset);
    strm_func(*strm, dir, user.data(), first, count);
  } else {
    fenix_assert(!dynamic);
    char* src         = dir == FENIX_SERIALIZE ? user.data() : stage.data();
    char* dst         = dir == FENIX_SERIALIZE ? stage.data() : user.data();
    size_t byte_count = count * elm_size;
    memcpy(dst + byte_offset, src + byte_offset, byte_count);
  }
}

size_t Serializer::size() const {
  fenix_assert(!user.is_bounded() || user.size() % elm_size == 0);
  fenix_assert(stage.size() % elm_size == 0);
  if (dynamic) return std::numeric_limits<size_t>::max();
  else if (!user.is_bounded()) return stage.size();
  else return std::min(stage.size(), user.size());
}

size_t Serializer::max_element() const {
  if (dynamic) return std::numeric_limits<size_t>::max();
  else return (size() / elm_size) - 1;
}

FILE* Serializer::get_file() {
  fenix_assert(has_file());
  return file->fp();
}

std::iostream* Serializer::get_stream() {
  fenix_assert(has_stream());
  return &strm.value();
}

Serializer::~Serializer() {
  if (dynamic) {
    size_t size;
    char* buf;
    bool mmap = false;

    if (use_file_func) {
      size = file->size();
      buf  = file->close_dynamic();
      mmap = false;
    } else {
      fenix_assert(use_strm_func);
      auto sbuf = dynamic_cast<detail::OMmapStreamBuf*>(strm->get_buf());

      size = sbuf->written_len();
      buf  = sbuf->release();
#ifdef FENIX_HAVE_MREMAP
      mmap = true;
#endif
    }

    if (mmap) stage.take_ownership_mmapped(buf, size);
    else stage.take_ownership(buf, size);
  }
}

Serializer::Serializer(Serializer&& o) : stage(o.stage) {
  use_file_func = o.use_file_func;
  file_func     = std::move(o.file_func);
  file          = std::move(o.file);
  use_strm_func = o.use_strm_func;
  strm_func     = std::move(o.strm_func);
  if (o.strm) strm.emplace(std::move(*o.strm));

  user     = o.user;
  dir      = o.dir;
  dynamic  = o.dynamic;
  elm_size = o.elm_size;

  o.dynamic = false;
}

} //namespace fenix::data::util
