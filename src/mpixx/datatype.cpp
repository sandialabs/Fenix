#include "fenix/mpixx/datatype.hpp"

#include <mpi.h>

#include <algorithm>
#include <cstring>

#include <stdexcept>

#include "fenix/mpixx/util.hpp"
#include "fenix.h"
#include "fenix_exception.hpp"
#include "fenix_opt.hpp"

namespace fenix::mpixx {

namespace detail {

// Combiner enumeration for serialization
// Compact binary representation: 1 byte per combiner
enum class TypeCombiner : uint8_t {
  // Special value for MPI_DATATYPE_NULL
  DATATYPE_NULL = 0xFF,

  // Builtin types (0x00 - 0x3F)
  BUILTIN_CHAR               = 0x00,
  BUILTIN_SHORT              = 0x01,
  BUILTIN_INT                = 0x02,
  BUILTIN_LONG               = 0x03,
  BUILTIN_LONG_LONG          = 0x04,
  BUILTIN_SIGNED_CHAR        = 0x05,
  BUILTIN_UNSIGNED_CHAR      = 0x06,
  BUILTIN_UNSIGNED_SHORT     = 0x07,
  BUILTIN_UNSIGNED           = 0x08,
  BUILTIN_UNSIGNED_LONG      = 0x09,
  BUILTIN_UNSIGNED_LONG_LONG = 0x0A,
  BUILTIN_FLOAT              = 0x0B,
  BUILTIN_DOUBLE             = 0x0C,
  BUILTIN_LONG_DOUBLE        = 0x0D,
  BUILTIN_BYTE               = 0x0E,
  BUILTIN_PACKED             = 0x0F,
  BUILTIN_WCHAR              = 0x10,
  BUILTIN_C_BOOL             = 0x11,
  BUILTIN_INT8_T             = 0x12,
  BUILTIN_INT16_T            = 0x13,
  BUILTIN_INT32_T            = 0x14,
  BUILTIN_INT64_T            = 0x15,
  BUILTIN_UINT8_T            = 0x16,
  BUILTIN_UINT16_T           = 0x17,
  BUILTIN_UINT32_T           = 0x18,
  BUILTIN_UINT64_T           = 0x19,
  BUILTIN_AINT               = 0x1A,
  BUILTIN_COUNT              = 0x1B,
  BUILTIN_OFFSET             = 0x1C,
  BUILTIN_FLOAT_INT          = 0x1D,
  BUILTIN_DOUBLE_INT         = 0x1E,
  BUILTIN_LONG_INT           = 0x1F,
  BUILTIN_SHORT_INT          = 0x20,
  BUILTIN_2INT               = 0x21,
  BUILTIN_LONG_DOUBLE_INT    = 0x22,

  // Derived combiners (0x40 - 0xFE)
  COMBINER_DUP            = 0x40,
  COMBINER_CONTIGUOUS     = 0x41,
  COMBINER_VECTOR         = 0x42,
  COMBINER_HVECTOR        = 0x43,
  COMBINER_INDEXED        = 0x44,
  COMBINER_HINDEXED       = 0x45,
  COMBINER_INDEXED_BLOCK  = 0x46,
  COMBINER_HINDEXED_BLOCK = 0x47,
  COMBINER_STRUCT         = 0x48,
  COMBINER_SUBARRAY       = 0x49,
  COMBINER_DARRAY         = 0x4A,
  COMBINER_RESIZED        = 0x4B,
};

// Lookup table for bidirectional mapping between MPI_Datatype and TypeCombiner
struct BuiltinMapping {
  MPI_Datatype mpi_type;
  TypeCombiner combiner;
};

static constexpr BuiltinMapping builtin_mappings[] = {
  {MPI_CHAR, TypeCombiner::BUILTIN_CHAR},
  {MPI_SHORT, TypeCombiner::BUILTIN_SHORT},
  {MPI_INT, TypeCombiner::BUILTIN_INT},
  {MPI_LONG, TypeCombiner::BUILTIN_LONG},
  {MPI_LONG_LONG, TypeCombiner::BUILTIN_LONG_LONG},
  {MPI_SIGNED_CHAR, TypeCombiner::BUILTIN_SIGNED_CHAR},
  {MPI_UNSIGNED_CHAR, TypeCombiner::BUILTIN_UNSIGNED_CHAR},
  {MPI_UNSIGNED_SHORT, TypeCombiner::BUILTIN_UNSIGNED_SHORT},
  {MPI_UNSIGNED, TypeCombiner::BUILTIN_UNSIGNED},
  {MPI_UNSIGNED_LONG, TypeCombiner::BUILTIN_UNSIGNED_LONG},
  {MPI_UNSIGNED_LONG_LONG, TypeCombiner::BUILTIN_UNSIGNED_LONG_LONG},
  {MPI_FLOAT, TypeCombiner::BUILTIN_FLOAT},
  {MPI_DOUBLE, TypeCombiner::BUILTIN_DOUBLE},
  {MPI_LONG_DOUBLE, TypeCombiner::BUILTIN_LONG_DOUBLE},
  {MPI_BYTE, TypeCombiner::BUILTIN_BYTE},
  {MPI_PACKED, TypeCombiner::BUILTIN_PACKED},
  {MPI_WCHAR, TypeCombiner::BUILTIN_WCHAR},
  {MPI_C_BOOL, TypeCombiner::BUILTIN_C_BOOL},
  {MPI_INT8_T, TypeCombiner::BUILTIN_INT8_T},
  {MPI_INT16_T, TypeCombiner::BUILTIN_INT16_T},
  {MPI_INT32_T, TypeCombiner::BUILTIN_INT32_T},
  {MPI_INT64_T, TypeCombiner::BUILTIN_INT64_T},
  {MPI_UINT8_T, TypeCombiner::BUILTIN_UINT8_T},
  {MPI_UINT16_T, TypeCombiner::BUILTIN_UINT16_T},
  {MPI_UINT32_T, TypeCombiner::BUILTIN_UINT32_T},
  {MPI_UINT64_T, TypeCombiner::BUILTIN_UINT64_T},
  {MPI_AINT, TypeCombiner::BUILTIN_AINT},
  {MPI_COUNT, TypeCombiner::BUILTIN_COUNT},
  {MPI_OFFSET, TypeCombiner::BUILTIN_OFFSET},
  {MPI_FLOAT_INT, TypeCombiner::BUILTIN_FLOAT_INT},
  {MPI_DOUBLE_INT, TypeCombiner::BUILTIN_DOUBLE_INT},
  {MPI_LONG_INT, TypeCombiner::BUILTIN_LONG_INT},
  {MPI_SHORT_INT, TypeCombiner::BUILTIN_SHORT_INT},
  {MPI_2INT, TypeCombiner::BUILTIN_2INT},
  {MPI_LONG_DOUBLE_INT, TypeCombiner::BUILTIN_LONG_DOUBLE_INT},
};

// Map builtin MPI_Datatype to our combiner enum
TypeCombiner builtin_to_combiner(MPI_Datatype type) {
  for (const auto& mapping : builtin_mappings) {
    if (type == mapping.mpi_type) {
      return mapping.combiner;
    }
  }
  FENIX_THROW(FENIX_ERROR_INTERN);
}

// Map our combiner enum to builtin MPI_Datatype
MPI_Datatype combiner_to_builtin(TypeCombiner combiner) {
  for (const auto& mapping : builtin_mappings) {
    if (mapping.combiner == combiner) {
      return mapping.mpi_type;
    }
  }
  FENIX_THROW(FENIX_ERROR_INTERN);
}

// Map MPI combiner constants to our enum
TypeCombiner mpi_combiner_to_enum(int mpi_combiner) {
  switch (mpi_combiner) {
  case MPI_COMBINER_DUP:
    return TypeCombiner::COMBINER_DUP;
  case MPI_COMBINER_CONTIGUOUS:
    return TypeCombiner::COMBINER_CONTIGUOUS;
  case MPI_COMBINER_VECTOR:
    return TypeCombiner::COMBINER_VECTOR;
  case MPI_COMBINER_HVECTOR:
    return TypeCombiner::COMBINER_HVECTOR;
  case MPI_COMBINER_INDEXED:
    return TypeCombiner::COMBINER_INDEXED;
  case MPI_COMBINER_HINDEXED:
    return TypeCombiner::COMBINER_HINDEXED;
  case MPI_COMBINER_INDEXED_BLOCK:
    return TypeCombiner::COMBINER_INDEXED_BLOCK;
  case MPI_COMBINER_HINDEXED_BLOCK:
    return TypeCombiner::COMBINER_HINDEXED_BLOCK;
  case MPI_COMBINER_STRUCT:
    return TypeCombiner::COMBINER_STRUCT;
  case MPI_COMBINER_SUBARRAY:
    return TypeCombiner::COMBINER_SUBARRAY;
  case MPI_COMBINER_DARRAY:
    return TypeCombiner::COMBINER_DARRAY;
  case MPI_COMBINER_RESIZED:
    return TypeCombiner::COMBINER_RESIZED;
  default:
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
}

} // namespace detail

// ========== Constructor/Destructor/Move Semantics ==========

Datatype& Datatype::operator=(Datatype&& other) noexcept {
  if (this != &other) {
    free();
    type_ = other.release();
  }
  return *this;
}

MPI_Datatype Datatype::release() noexcept {
  MPI_Datatype tmp = type_;
  type_            = MPI_DATATYPE_NULL;
  return tmp;
}

// ========== Builtin Type Detection ==========

bool Datatype::is_builtin_type(MPI_Datatype type) noexcept {
  if (type == MPI_DATATYPE_NULL) return false;

  int num_integers, num_addresses, num_datatypes, combiner;
  int err = MPI_Type_get_envelope(
    type, &num_integers, &num_addresses, &num_datatypes, &combiner
  );
  fenix_assert(err == MPI_SUCCESS);

  return combiner == MPI_COMBINER_NAMED;
}

bool Datatype::is_builtin() const noexcept { return is_builtin_type(type_); }

void Datatype::free() {
  if (type_ != MPI_DATATYPE_NULL && mpi_active() && !is_builtin()) {
    MPI_Type_free(&type_);
  }
  type_ = MPI_DATATYPE_NULL;
}

// ========== MPI Datatype Operations ==========

void Datatype::commit() {
  int err = MPI_Type_commit(&type_);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
}

int Datatype::size() const {
  int sz;
  int err = MPI_Type_size(type_, &sz);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return sz;
}

int Datatype::extent() const {
  MPI_Aint lb, ext;
  get_extent(&lb, &ext);
  return static_cast<int>(ext);
}

void Datatype::get_extent(MPI_Aint* lb, MPI_Aint* extent) const {
  int err = MPI_Type_get_extent(type_, lb, extent);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
}

void Datatype::get_true_extent(MPI_Aint* true_lb, MPI_Aint* true_extent) const {
  int err = MPI_Type_get_true_extent(type_, true_lb, true_extent);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
}

// ========== Type Construction Factory Methods ==========

Datatype Datatype::contiguous(int count, MPI_Datatype oldtype) {
  MPI_Datatype newtype;
  int err = MPI_Type_contiguous(count, oldtype, &newtype);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Datatype(newtype);
}

Datatype Datatype::vector(
  int count, int blocklength, int stride, MPI_Datatype oldtype
) {

  MPI_Datatype newtype;
  int err = MPI_Type_vector(count, blocklength, stride, oldtype, &newtype);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Datatype(newtype);
}

Datatype Datatype::hvector(
  int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype
) {
  MPI_Datatype newtype;
  int err =
    MPI_Type_create_hvector(count, blocklength, stride, oldtype, &newtype);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Datatype(newtype);
}

Datatype Datatype::indexed(
  int count, const int* array_of_blocklengths,
  const int* array_of_displacements, MPI_Datatype oldtype
) {
  MPI_Datatype newtype;
  int err = MPI_Type_indexed(
    count, array_of_blocklengths, array_of_displacements, oldtype, &newtype
  );
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Datatype(newtype);
}

Datatype Datatype::hindexed(
  int count, const int* array_of_blocklengths,
  const MPI_Aint* array_of_displacements, MPI_Datatype oldtype
) {
  MPI_Datatype newtype;
  int err = MPI_Type_create_hindexed(
    count, array_of_blocklengths, array_of_displacements, oldtype, &newtype
  );
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Datatype(newtype);
}

Datatype Datatype::indexed_block(
  int count, int blocklength, const int* array_of_displacements,
  MPI_Datatype oldtype
) {
  MPI_Datatype newtype;
  int err = MPI_Type_create_indexed_block(
    count, blocklength, array_of_displacements, oldtype, &newtype
  );
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Datatype(newtype);
}

Datatype Datatype::hindexed_block(
  int count, int blocklength, const MPI_Aint* array_of_displacements,
  MPI_Datatype oldtype
) {
  MPI_Datatype newtype;
  int err = MPI_Type_create_hindexed_block(
    count, blocklength, array_of_displacements, oldtype, &newtype
  );
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Datatype(newtype);
}

Datatype Datatype::create_struct(
  int count, const int* array_of_blocklengths,
  const MPI_Aint* array_of_displacements, const MPI_Datatype* array_of_types
) {
  MPI_Datatype newtype;
  int err = MPI_Type_create_struct(
    count, array_of_blocklengths, array_of_displacements, array_of_types,
    &newtype
  );
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Datatype(newtype);
}

Datatype Datatype::subarray(
  int ndims, const int* array_of_sizes, const int* array_of_subsizes,
  const int* array_of_starts, int order, MPI_Datatype oldtype
) {
  MPI_Datatype newtype;
  int err = MPI_Type_create_subarray(
    ndims, array_of_sizes, array_of_subsizes, array_of_starts, order, oldtype,
    &newtype
  );
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Datatype(newtype);
}

Datatype Datatype::darray(
  int size, int rank, int ndims, const int* array_of_gsizes,
  const int* array_of_distribs, const int* array_of_dargs,
  const int* array_of_psizes, int order, MPI_Datatype oldtype
) {
  MPI_Datatype newtype;
  int err = MPI_Type_create_darray(
    size, rank, ndims, array_of_gsizes, array_of_distribs, array_of_dargs,
    array_of_psizes, order, oldtype, &newtype
  );
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Datatype(newtype);
}

Datatype Datatype::resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent) {
  MPI_Datatype newtype;
  int err = MPI_Type_create_resized(oldtype, lb, extent, &newtype);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Datatype(newtype);
}

Datatype Datatype::dup(MPI_Datatype oldtype) {
  MPI_Datatype newtype;
  int err = MPI_Type_dup(oldtype, &newtype);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Datatype(newtype);
}

// ========== Serialization ==========

struct Datatype::TypeInfo {
  int combiner;
  std::vector<int> integers;
  std::vector<MPI_Aint> addresses;
  std::vector<MPI_Datatype> datatypes;
  std::vector<Datatype> datatype_owners; // RAII wrappers to free datatypes
};

Datatype::TypeInfo Datatype::introspect(MPI_Datatype type) {
  TypeInfo info;

  // Get envelope to determine buffer sizes
  int num_ints, num_addrs, num_dtypes;
  int err = MPI_Type_get_envelope(
    type, &num_ints, &num_addrs, &num_dtypes, &info.combiner
  );
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }

  // Allocate buffers
  info.integers.resize(num_ints);
  info.addresses.resize(num_addrs);
  info.datatypes.resize(num_dtypes);

  // Get contents
  if (num_ints > 0 || num_addrs > 0 || num_dtypes > 0) {
    err = MPI_Type_get_contents(
      type, num_ints, num_addrs, num_dtypes, info.integers.data(),
      info.addresses.data(), info.datatypes.data()
    );
    if (err != MPI_SUCCESS) {
      FENIX_THROW(FENIX_ERROR_INTERN);
    }
  }

  // Transfer ownership of MPI_Datatype handles to RAII wrappers
  // MPI_Type_get_contents creates new handles that must be freed
  info.datatype_owners.reserve(num_dtypes);
  for (int i = 0; i < num_dtypes; i++) {
    info.datatype_owners.emplace_back(info.datatypes[i]);
  }

  return info;
}

// Helper to write value to buffer with optional type conversion
// SerialT: the type to serialize as (e.g., int32_t, int64_t)
// SrcT: the source value type (deduced, e.g., int, MPI_Aint)
template <typename SerialT, typename SrcT = SerialT>
void write_val(std::vector<uint8_t>& buffer, SrcT value) {
  SerialT serialized = static_cast<SerialT>(value);
  buffer.insert(
    buffer.end(), reinterpret_cast<const uint8_t*>(&serialized),
    reinterpret_cast<const uint8_t*>(&serialized) + sizeof(SerialT)
  );
}

// Helper to read value from buffer with optional type conversion
// SerialT: the type to deserialize from (e.g., int32_t, int64_t)
// DstT: the destination value type (defaults to SerialT)
template <typename SerialT, typename DstT = SerialT>
DstT read_val(const uint8_t*& ptr, const uint8_t* end) {
  if (ptr + sizeof(SerialT) > end) {
    FENIX_THROW("Unexpected end of serialized datatype buffer");
  }
  SerialT value;
  std::memcpy(&value, ptr, sizeof(SerialT));
  ptr += sizeof(SerialT);
  return static_cast<DstT>(value);
}

// Helper to write array to buffer with automatic type conversion
// SerialT: the type to serialize as (e.g., int32_t, int64_t)
// SrcT: the source array type (e.g., int, MPI_Aint)
template <typename SerialT, typename SrcT>
void write_array(std::vector<uint8_t>& buffer, const SrcT* values, int count) {
  for (int i = 0; i < count; ++i) {
    write_val<SerialT>(buffer, values[i]);
  }
}

// Helper to read array from buffer with automatic type conversion
// SerialT: the type to deserialize from (e.g., int32_t, int64_t)
// DstT: the destination array type (e.g., int, MPI_Aint)
template <typename SerialT, typename DstT>
void read_array(
  const uint8_t*& ptr, const uint8_t* end, DstT* values, int count
) {
  for (int i = 0; i < count; ++i) {
    values[i] = read_val<SerialT, DstT>(ptr, end);
  }
}

// Helper to read array from buffer and return as vector
// DstT: the destination element type (e.g., int, MPI_Aint)
// SerialT: the type to deserialize from (e.g., int32_t, int64_t)
template <typename DstT, typename SerialT>
std::vector<DstT> read_vector(
  const uint8_t*& ptr, const uint8_t* end, int count
) {
  std::vector<DstT> result(count);
  read_array<SerialT>(ptr, end, result.data(), count);
  return result;
}

void Datatype::serialize_recursive(
  MPI_Datatype type, std::vector<uint8_t>& buffer
) {
  // Handle MPI_DATATYPE_NULL specially
  if (type == MPI_DATATYPE_NULL) {
    buffer.push_back(static_cast<uint8_t>(detail::TypeCombiner::DATATYPE_NULL));
    return;
  }

  // Introspect the type
  TypeInfo info = introspect(type);

  // Handle builtin types
  if (info.combiner == MPI_COMBINER_NAMED) {
    auto combiner_enum = detail::builtin_to_combiner(type);
    buffer.push_back(static_cast<uint8_t>(combiner_enum));
    return;
  }

  // Convert MPI combiner to our enum and write it
  auto combiner_enum = detail::mpi_combiner_to_enum(info.combiner);
  buffer.push_back(static_cast<uint8_t>(combiner_enum));

  switch (info.combiner) {
  case MPI_COMBINER_DUP:
    serialize_recursive(info.datatypes[0], buffer);
    break;

  case MPI_COMBINER_CONTIGUOUS:
  case MPI_COMBINER_VECTOR:
  case MPI_COMBINER_INDEXED:
  case MPI_COMBINER_INDEXED_BLOCK:
  case MPI_COMBINER_SUBARRAY:
  case MPI_COMBINER_DARRAY:
    write_array<int32_t>(buffer, info.integers.data(), info.integers.size());
    serialize_recursive(info.datatypes[0], buffer);
    break;

  case MPI_COMBINER_HVECTOR:
  case MPI_COMBINER_HINDEXED:
  case MPI_COMBINER_HINDEXED_BLOCK:
    write_array<int32_t>(buffer, info.integers.data(), info.integers.size());
    write_array<int64_t>(buffer, info.addresses.data(), info.addresses.size());
    serialize_recursive(info.datatypes[0], buffer);
    break;

  case MPI_COMBINER_STRUCT:
    write_array<int32_t>(buffer, info.integers.data(), info.integers.size());
    write_array<int64_t>(buffer, info.addresses.data(), info.addresses.size());
    for (size_t i = 0; i < info.datatypes.size(); i++) {
      serialize_recursive(info.datatypes[i], buffer);
    }
    break;

  case MPI_COMBINER_RESIZED:
    write_array<int64_t>(buffer, info.addresses.data(), info.addresses.size());
    serialize_recursive(info.datatypes[0], buffer);
    break;

  default:
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
}

std::vector<uint8_t> Datatype::serialize() const {
  fenix_assert(mpi_active(), "MPI not initialized");
  std::vector<uint8_t> buffer;
  uint32_t magic = 0x46445450; // 'FDTP'
  write_val<uint32_t>(buffer, magic);
  serialize_recursive(type_, buffer);
  return buffer;
}

// ========== Deserialization ==========

Datatype Datatype::deserialize_recursive(
  const uint8_t*& ptr, const uint8_t* end
) {
  if (ptr >= end) {
    FENIX_THROW("Unexpected end of serialized datatype buffer");
  }

  // Read combiner byte
  auto combiner = static_cast<detail::TypeCombiner>(*ptr++);

  // Handle MPI_DATATYPE_NULL
  if (combiner == detail::TypeCombiner::DATATYPE_NULL) {
    return Datatype(MPI_DATATYPE_NULL);
  }

  // Handle builtin types
  if (static_cast<uint8_t>(combiner) < 0x40) {
    return Datatype(detail::combiner_to_builtin(combiner));
  }

  // Handle derived types
  switch (combiner) {
  case detail::TypeCombiner::COMBINER_DUP: {
    Datatype child = deserialize_recursive(ptr, end);
    return Datatype::dup(child);
  }

  case detail::TypeCombiner::COMBINER_CONTIGUOUS: {
    int32_t count  = read_val<int32_t>(ptr, end);
    Datatype child = deserialize_recursive(ptr, end);
    return Datatype::contiguous(count, child);
  }

  case detail::TypeCombiner::COMBINER_VECTOR: {
    int32_t count       = read_val<int32_t>(ptr, end);
    int32_t blocklength = read_val<int32_t>(ptr, end);
    int32_t stride      = read_val<int32_t>(ptr, end);
    Datatype child      = deserialize_recursive(ptr, end);
    return Datatype::vector(count, blocklength, stride, child);
  }

  case detail::TypeCombiner::COMBINER_HVECTOR: {
    int32_t count       = read_val<int32_t>(ptr, end);
    int32_t blocklength = read_val<int32_t>(ptr, end);
    int64_t stride      = read_val<int64_t>(ptr, end);
    Datatype child      = deserialize_recursive(ptr, end);
    return Datatype::hvector(
      count, blocklength, static_cast<MPI_Aint>(stride), child
    );
  }

  case detail::TypeCombiner::COMBINER_INDEXED: {
    int32_t count      = read_val<int32_t>(ptr, end);
    auto blocklengths  = read_vector<int, int32_t>(ptr, end, count);
    auto displacements = read_vector<int, int32_t>(ptr, end, count);
    Datatype child     = deserialize_recursive(ptr, end);
    return Datatype::indexed(
      count, blocklengths.data(), displacements.data(), child
    );
  }

  case detail::TypeCombiner::COMBINER_HINDEXED: {
    int32_t count      = read_val<int32_t>(ptr, end);
    auto blocklengths  = read_vector<int, int32_t>(ptr, end, count);
    auto displacements = read_vector<MPI_Aint, int64_t>(ptr, end, count);
    Datatype child     = deserialize_recursive(ptr, end);
    return Datatype::hindexed(
      count, blocklengths.data(), displacements.data(), child
    );
  }

  case detail::TypeCombiner::COMBINER_INDEXED_BLOCK: {
    int32_t count       = read_val<int32_t>(ptr, end);
    int32_t blocklength = read_val<int32_t>(ptr, end);
    auto displacements  = read_vector<int, int32_t>(ptr, end, count);
    Datatype child      = deserialize_recursive(ptr, end);
    return Datatype::indexed_block(
      count, blocklength, displacements.data(), child
    );
  }

  case detail::TypeCombiner::COMBINER_HINDEXED_BLOCK: {
    int32_t count       = read_val<int32_t>(ptr, end);
    int32_t blocklength = read_val<int32_t>(ptr, end);
    auto displacements  = read_vector<MPI_Aint, int64_t>(ptr, end, count);
    Datatype child      = deserialize_recursive(ptr, end);
    return Datatype::hindexed_block(
      count, blocklength, displacements.data(), child
    );
  }

  case detail::TypeCombiner::COMBINER_STRUCT: {
    int32_t count      = read_val<int32_t>(ptr, end);
    auto blocklengths  = read_vector<int, int32_t>(ptr, end, count);
    auto displacements = read_vector<MPI_Aint, int64_t>(ptr, end, count);
    std::vector<Datatype> children;
    children.reserve(count);
    for (int i = 0; i < count; i++) {
      children.push_back(deserialize_recursive(ptr, end));
    }
    std::vector<MPI_Datatype> types(count);
    for (int i = 0; i < count; i++) {
      types[i] = children[i];
    }
    return Datatype::create_struct(
      count, blocklengths.data(), displacements.data(), types.data()
    );
  }

  case detail::TypeCombiner::COMBINER_SUBARRAY: {
    int32_t ndims  = read_val<int32_t>(ptr, end);
    auto sizes     = read_vector<int, int32_t>(ptr, end, ndims);
    auto subsizes  = read_vector<int, int32_t>(ptr, end, ndims);
    auto starts    = read_vector<int, int32_t>(ptr, end, ndims);
    int32_t order  = read_val<int32_t>(ptr, end);
    Datatype child = deserialize_recursive(ptr, end);
    return Datatype::subarray(
      ndims, sizes.data(), subsizes.data(), starts.data(), order, child
    );
  }

  case detail::TypeCombiner::COMBINER_DARRAY: {
    int32_t size   = read_val<int32_t>(ptr, end);
    int32_t rank   = read_val<int32_t>(ptr, end);
    int32_t ndims  = read_val<int32_t>(ptr, end);
    auto gsizes    = read_vector<int, int32_t>(ptr, end, ndims);
    auto distribs  = read_vector<int, int32_t>(ptr, end, ndims);
    auto dargs     = read_vector<int, int32_t>(ptr, end, ndims);
    auto psizes    = read_vector<int, int32_t>(ptr, end, ndims);
    int32_t order  = read_val<int32_t>(ptr, end);
    Datatype child = deserialize_recursive(ptr, end);
    return Datatype::darray(
      size, rank, ndims, gsizes.data(), distribs.data(), dargs.data(),
      psizes.data(), order, child
    );
  }

  case detail::TypeCombiner::COMBINER_RESIZED: {
    int64_t lb     = read_val<int64_t>(ptr, end);
    int64_t extent = read_val<int64_t>(ptr, end);
    Datatype child = deserialize_recursive(ptr, end);
    return Datatype::resized(
      child, static_cast<MPI_Aint>(lb), static_cast<MPI_Aint>(extent)
    );
  }

  default:
    FENIX_THROW(FENIX_ERROR_INTERN);
    return Datatype(MPI_DATATYPE_NULL); // Unreachable
  }
}

Datatype Datatype::deserialize(const std::vector<uint8_t>& buffer) {
  return deserialize(buffer.data(), buffer.size());
}

Datatype Datatype::deserialize(const uint8_t* data, size_t size) {
  fenix_assert(data != nullptr, "Data pointer cannot be null");
  fenix_assert(mpi_active(), "MPI not initialized");

  if (size < 4) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }

  const uint8_t* ptr = data;
  const uint8_t* end = data + size;

  // Validate magic number
  uint32_t magic = read_val<uint32_t>(ptr, end);

  if (magic != 0x46445450) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }

  // Recursively deserialize
  Datatype type = deserialize_recursive(ptr, end);

  // Commit the type before returning (unless it's MPI_DATATYPE_NULL or builtin)
  if (type.get() != MPI_DATATYPE_NULL && !type.is_builtin()) {
    type.commit();
  }

  return type;
}

} // namespace fenix::mpixx
