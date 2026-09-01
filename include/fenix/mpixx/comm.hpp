#ifndef FENIX_MPIXX_COMM_HPP
#define FENIX_MPIXX_COMM_HPP

#include <mpi.h>
#include <utility>

#include "fenix/mpixx/util.hpp"

namespace fenix::mpixx {

// RAII wrapper for MPI_Comm with move-only semantics
// Owns an MPI_Comm handle and automatically frees it on destruction.
// Accessors check MPI initialization state and return MPI_COMM_NULL
// if MPI is not initialized or has been finalized.
class Comm {
 public:
  explicit Comm(MPI_Comm comm) noexcept : comm_(comm) {}
  Comm() noexcept : Comm(MPI_COMM_NULL) {}
  virtual ~Comm() { free(); }

  // frees old comm before taking ownership
  Comm& operator=(Comm&& o);
  Comm& operator=(MPI_Comm c) { return *this = Comm(c); }
  Comm(Comm&& o) noexcept { *this = std::move(o); }

  // Disable copy
  Comm(const Comm&)            = delete;
  Comm& operator=(const Comm&) = delete;

  MPI_Comm get() const noexcept { return mpi_active() ? comm_ : MPI_COMM_NULL; }

  // Implicit conversion to MPI_Comm
  operator MPI_Comm() const noexcept { return get(); }

  explicit operator bool() const noexcept { return get() != MPI_COMM_NULL; }

  // Release ownership of the communicator without freeing it
  MPI_Comm release() noexcept;

  // Basic MPI_Comm function overloads
  int size() const;
  int rank() const;
  bool is_revoked() const;
  int revoke();
  virtual void free(); // safe even if comm_ is MPI_COMM_NULL

  // Static versions taking raw MPI_Comm
  static int size(MPI_Comm comm);
  static int rank(MPI_Comm comm);
  static bool is_revoked(MPI_Comm comm);
  static int revoke(MPI_Comm comm);

  // MPI_Comm creation overloads
  Comm dup() const;
  Comm dup_with_info(MPI_Info info) const;

  Comm create(MPI_Group group) const;
  Comm create_group(MPI_Group group, int tag) const;

  Comm split(int color, int key) const;
  Comm split_type(int split_type, int key, MPI_Info info) const;

  Comm intercomm_create(
    int local_leader, MPI_Comm peer_comm, int remote_leader, int tag
  ) const;

  Comm shrink() const;

  // Static MPI_Comm creation overloads (taking an input comm)
  static Comm dup(MPI_Comm comm);
  static Comm dup_with_info(MPI_Comm comm, MPI_Info info);

  static Comm create(MPI_Comm comm, MPI_Group group);
  static Comm create_group(MPI_Comm comm, MPI_Group group, int tag);

  static Comm split(MPI_Comm comm, int color, int key);
  static Comm split_type(MPI_Comm comm, int split_type, int key, MPI_Info info);

  static Comm intercomm_create(
    MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm,
    int remote_leader, int tag
  );

  static Comm shrink(MPI_Comm comm);

 private:
  MPI_Comm comm_ = MPI_COMM_NULL;
};

// Non-owning reference to an MPI_Comm
// Does not free the communicator on destruction, only releases ownership.
// Useful for storing communicators that are owned elsewhere.
// Unlike Comm, CommRef is copyable since it doesn't own the resource.
class CommRef : public Comm {
 public:
  // Implicit constructor from MPI_Comm
  CommRef(MPI_Comm comm = MPI_COMM_NULL) : Comm(comm) {}

  // Construct from Comm (non-owning reference)
  CommRef(const Comm& c) : Comm(c.get()) {}

  // Copy operations (allowed for non-owning reference)
  CommRef(const CommRef& o) : Comm(o.get()) {}
  CommRef& operator=(const CommRef& o) {
    *this = o.get();
    return *this;
  }

  // Assign from Comm (non-owning reference)
  CommRef& operator=(const Comm& c) {
    *this = c.get();
    return *this;
  }

  // Move assignment - release old comm without freeing it
  CommRef& operator=(CommRef&& o) {
    if (this != &o) {
      (void)release();     // Just release the old one, don't free
      *this = o.release(); // Assign the new comm via operator=(MPI_Comm)
    }
    return *this;
  }
  CommRef& operator=(MPI_Comm c) {
    (void)release(); // Just release the old one, don't free
    // Call base class constructor via placement new to set new comm
    new (this) Comm(c);
    return *this;
  }

  ~CommRef() override { (void)release(); }
};

} // namespace fenix::mpixx

#endif // FENIX_MPIXX_COMM_HPP
