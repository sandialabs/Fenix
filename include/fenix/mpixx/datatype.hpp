#ifndef FENIX_MPIXX_DATATYPE_HPP
#define FENIX_MPIXX_DATATYPE_HPP

#include <mpi.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "fenix/mpixx/util.hpp"

namespace fenix::mpixx {

// RAII wrapper for MPI_Datatype with move-only semantics
// Owns an MPI_Datatype handle and automatically frees it on destruction.
// Does NOT free builtin MPI datatypes (MPI_INT, MPI_DOUBLE, etc.).
// Accessors check MPI initialization state and return MPI_DATATYPE_NULL
// if MPI is not initialized or has been finalized.
class Datatype {
 public:
  // Construct from existing MPI_Datatype (takes ownership)
  explicit Datatype(MPI_Datatype type) noexcept : type_(type) {}

  // Default constructor creates MPI_DATATYPE_NULL
  Datatype() noexcept : Datatype(MPI_DATATYPE_NULL) {}

  // Destructor automatically frees non-builtin types
  virtual ~Datatype() { free(); }

  // Move semantics (frees old type before taking ownership)
  Datatype& operator=(Datatype&& o) noexcept;
  Datatype& operator=(MPI_Datatype type) { return *this = Datatype(type); }
  Datatype(Datatype&& o) noexcept { *this = std::move(o); }

  // Delete copy operations (move-only)
  Datatype(const Datatype&) = delete;
  Datatype& operator=(const Datatype&) = delete;

  // Accessors
  MPI_Datatype get() const noexcept {
    return mpi_active() ? type_ : MPI_DATATYPE_NULL;
  }

  // Implicit conversion to MPI_Datatype
  operator MPI_Datatype() const noexcept { return get(); }

  explicit operator bool() const noexcept { return get() != MPI_DATATYPE_NULL; }

  // Release ownership without freeing
  MPI_Datatype release() noexcept;

  // MPI datatype operations
  void commit();
  int size() const;
  int extent() const;
  void get_extent(MPI_Aint* lb, MPI_Aint* extent) const;
  void get_true_extent(MPI_Aint* true_lb, MPI_Aint* true_extent) const;

  // Check if this is a builtin type
  bool is_builtin() const noexcept;

  // Free the datatype (safe even if type_ is MPI_DATATYPE_NULL or builtin)
  virtual void free();

  // ========== Type Construction Factory Methods ==========

  // Create contiguous type
  static Datatype contiguous(int count, MPI_Datatype oldtype);

  // Create vector type (regular strided pattern with element strides)
  static Datatype vector(
    int count, int blocklength, int stride, MPI_Datatype oldtype
  );

  // Create hvector (vector with byte stride)
  static Datatype hvector(
    int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype
  );

  // Create indexed type (variable blocks and displacements in element units)
  static Datatype indexed(
    int count,
    const int* array_of_blocklengths,
    const int* array_of_displacements,
    MPI_Datatype oldtype
  );

  // Create hindexed (indexed with byte displacements)
  static Datatype hindexed(
    int count,
    const int* array_of_blocklengths,
    const MPI_Aint* array_of_displacements,
    MPI_Datatype oldtype
  );

  // Create indexed_block (all blocks same length, element displacements)
  static Datatype indexed_block(
    int count,
    int blocklength,
    const int* array_of_displacements,
    MPI_Datatype oldtype
  );

  // Create hindexed_block (indexed_block with byte displacements)
  static Datatype hindexed_block(
    int count,
    int blocklength,
    const MPI_Aint* array_of_displacements,
    MPI_Datatype oldtype
  );

  // Create struct type (heterogeneous)
  static Datatype create_struct(
    int count,
    const int* array_of_blocklengths,
    const MPI_Aint* array_of_displacements,
    const MPI_Datatype* array_of_types
  );

  // Create subarray type (multidimensional subarray)
  static Datatype subarray(
    int ndims,
    const int* array_of_sizes,
    const int* array_of_subsizes,
    const int* array_of_starts,
    int order,
    MPI_Datatype oldtype
  );

  // Create darray (distributed array) type
  static Datatype darray(
    int size,
    int rank,
    int ndims,
    const int* array_of_gsizes,
    const int* array_of_distribs,
    const int* array_of_dargs,
    const int* array_of_psizes,
    int order,
    MPI_Datatype oldtype
  );

  // Create resized type (change lower bound and extent)
  static Datatype resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent);

  // Create duplicate type
  static Datatype dup(MPI_Datatype oldtype);

  // ========== Serialization ==========

  // Serialize this datatype to a portable byte buffer
  // Returns vector of bytes that can be sent to another rank
  std::vector<uint8_t> serialize() const;

  // Deserialize from byte buffer to reconstruct datatype
  // Throws if buffer is invalid or deserialization fails
  static Datatype deserialize(const std::vector<uint8_t>& buffer);
  static Datatype deserialize(const uint8_t* data, size_t size);

 private:
  MPI_Datatype type_ = MPI_DATATYPE_NULL;

  // Helper: Check if a given MPI_Datatype is builtin
  static bool is_builtin_type(MPI_Datatype type) noexcept;

  // Serialization helpers (implementation details in .cpp)
  struct TypeInfo;
  static TypeInfo introspect(MPI_Datatype type);
  static void serialize_recursive(
    MPI_Datatype type, std::vector<uint8_t>& buffer
  );
  static Datatype deserialize_recursive(
    const uint8_t*& ptr, const uint8_t* end
  );
};

// Non-owning reference to an MPI_Datatype
// Does not free the datatype on destruction, only releases ownership.
// Useful for storing datatypes that are owned elsewhere (e.g., builtins).
// Unlike Datatype, DatatypeRef is copyable since it doesn't own the resource.
class DatatypeRef : public Datatype {
 public:
  // Implicit constructor from MPI_Datatype
  DatatypeRef(MPI_Datatype type = MPI_DATATYPE_NULL) : Datatype(type) {}

  // Construct from Datatype (non-owning reference)
  DatatypeRef(const Datatype& dt) : Datatype(dt.get()) {}

  // Copy operations (allowed for non-owning reference)
  DatatypeRef(const DatatypeRef& other) : Datatype(other.get()) {}
  DatatypeRef& operator=(const DatatypeRef& other) {
    *this = other.get();
    return *this;
  }

  // Assign from Datatype (non-owning reference)
  DatatypeRef& operator=(const Datatype& dt) {
    *this = dt.get();
    return *this;
  }

  // Move assignment - release old type without freeing it
  DatatypeRef& operator=(DatatypeRef&& other) {
    if (this != &other) {
      (void)release();        // Just release the old one, don't free
      *this = other.release(); // Assign the new type via operator=(MPI_Datatype)
    }
    return *this;
  }

  DatatypeRef& operator=(MPI_Datatype type) {
    (void)release(); // Just release the old one, don't free
    // Call base class constructor via placement new to set new type
    new (this) Datatype(type);
    return *this;
  }

  ~DatatypeRef() override { (void)release(); }
};

} // namespace fenix::mpixx

#endif // FENIX_MPIXX_DATATYPE_HPP
