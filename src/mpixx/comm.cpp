#include "fenix_opt.hpp"
#include "fenix/mpixx/comm.hpp"
#include "fenix/mpixx/util.hpp"
#include "fenix/mpixx/status.hpp"

#include <mpi.h>
#ifndef MPICH_VERSION
#include <mpi-ext.h>
#endif

namespace fenix::mpixx {

Comm& Comm::operator=(Comm&& o) {
  if (this != &o) {
    free();
    comm_ = o.release();
  }
  return *this;
}

MPI_Comm Comm::release() noexcept {
  MPI_Comm tmp = comm_;
  comm_        = MPI_COMM_NULL;
  return tmp;
}

int Comm::size() const {
  fenix_assert(*this);
  int size_val;
  MPI_Comm_size(get(), &size_val);
  return size_val;
}

int Comm::rank() const {
  fenix_assert(*this);
  int rank_val;
  MPI_Comm_rank(get(), &rank_val);
  return rank_val;
}

int Comm::revoke() {
  fenix_assert(*this);
  return MPIX_Comm_revoke(get());
}

bool Comm::is_revoked() const {
  fenix_assert(*this);
  int flag;
  Status ret = MPIX_Comm_is_revoked(get(), &flag);
  return ret && flag;
}

void Comm::free() {
  fenix_assert(comm_ != MPI_COMM_WORLD);
  fenix_assert(comm_ != MPI_COMM_SELF);
  if (*this) MPI_Comm_free(&comm_);
  comm_ = MPI_COMM_NULL;
}

Group Comm::group() const { return group(get()); }

// Member functions for communicator creation

Comm Comm::dup() const { return dup(get()); }
Comm Comm::dup_with_info(MPI_Info i) const { return dup_with_info(get(), i); }

Comm Comm::create(MPI_Group g) const { return create(get(), g); }
Comm Comm::create_group(MPI_Group g, int t) const {
  return create_group(get(), g, t);
}

Comm Comm::split(int c, int k) const { return split(get(), c, k); }
Comm Comm::split_type(int t, int k, MPI_Info i) const {
  return split_type(get(), t, k, i);
}

Comm Comm::intercomm_create(int ll, MPI_Comm pc, int rl, int t) const {
  return intercomm_create(get(), ll, pc, rl, t);
}

Comm Comm::shrink() const { return shrink(get()); }

// Static functions for communicator creation

Comm Comm::dup(MPI_Comm comm) {
  fenix_assert(comm != MPI_COMM_NULL);
  fenix_assert(mpi_active());

  MPI_Comm new_comm;
  Status ret = MPI_Comm_dup(comm, &new_comm);
  return ret ? Comm(new_comm) : Comm();
}

Comm Comm::dup_with_info(MPI_Comm comm, MPI_Info info) {
  fenix_assert(comm != MPI_COMM_NULL);
  fenix_assert(mpi_active());

  MPI_Comm new_comm;
  Status ret = MPI_Comm_dup_with_info(comm, info, &new_comm);
  return ret ? Comm(new_comm) : Comm();
}

Comm Comm::create(MPI_Comm comm, MPI_Group group) {
  fenix_assert(comm != MPI_COMM_NULL);
  fenix_assert(mpi_active());

  MPI_Comm new_comm;
  Status ret = MPI_Comm_create(comm, group, &new_comm);
  return ret ? Comm(new_comm) : Comm();
}

Comm Comm::create_group(MPI_Comm comm, MPI_Group group, int tag) {
  fenix_assert(comm != MPI_COMM_NULL);
  fenix_assert(mpi_active());

  MPI_Comm new_comm;
  Status ret = MPI_Comm_create_group(comm, group, tag, &new_comm);
  return ret ? Comm(new_comm) : Comm();
}

Comm Comm::split(MPI_Comm comm, int color, int key) {
  fenix_assert(comm != MPI_COMM_NULL);
  fenix_assert(mpi_active());

  MPI_Comm new_comm;
  Status ret = MPI_Comm_split(comm, color, key, &new_comm);
  return ret ? Comm(new_comm) : Comm();
}

Comm Comm::split_type(MPI_Comm comm, int split_type, int key, MPI_Info info) {
  fenix_assert(comm != MPI_COMM_NULL);
  fenix_assert(info != MPI_INFO_NULL);
  fenix_assert(mpi_active());

  MPI_Comm new_comm;
  Status ret = MPI_Comm_split_type(comm, split_type, key, info, &new_comm);
  return ret ? Comm(new_comm) : Comm();
}

Comm Comm::intercomm_create(
  MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader,
  int tag
) {
  fenix_assert(local_comm != MPI_COMM_NULL);
  fenix_assert(peer_comm != MPI_COMM_NULL);
  fenix_assert(mpi_active());

  MPI_Comm new_comm;
  Status ret = MPI_Intercomm_create(
    local_comm, local_leader, peer_comm, remote_leader, tag, &new_comm
  );
  return ret ? Comm(new_comm) : Comm();
}

Comm Comm::shrink(MPI_Comm comm) {
  fenix_assert(comm != MPI_COMM_NULL);
  fenix_assert(mpi_active());

  MPI_Comm new_comm;
  MPIX_Comm_shrink(comm, &new_comm);
  return Comm(new_comm);
}

// Static versions taking raw MPI_Comm

int Comm::size(MPI_Comm comm) {
  fenix_assert(comm != MPI_COMM_NULL);
  fenix_assert(mpi_active());
  int size_val;
  MPI_Comm_size(comm, &size_val);
  return size_val;
}

int Comm::rank(MPI_Comm comm) {
  fenix_assert(comm != MPI_COMM_NULL);
  fenix_assert(mpi_active());
  int rank_val;
  MPI_Comm_rank(comm, &rank_val);
  return rank_val;
}

int Comm::revoke(MPI_Comm comm) {
  fenix_assert(comm != MPI_COMM_NULL);
  fenix_assert(mpi_active());
  return MPIX_Comm_revoke(comm);
}

Group Comm::group(MPI_Comm comm) { return Group::from_comm(comm); }

bool Comm::is_revoked(MPI_Comm comm) {
  fenix_assert(comm != MPI_COMM_NULL);
  fenix_assert(mpi_active());
  int flag;
  Status ret = MPIX_Comm_is_revoked(comm, &flag);
  return ret && flag;
}

// Private helper functions

} // namespace fenix::mpixx
