#include <fenix.hpp>
#include <fenix_opt.hpp>
#include <fenix/mpixx/util.hpp>
#include <mpi.h>
#include <mpi-ext.h>
#include <signal.h>

namespace comm_registration_test {

inline MPI_Comm fenix_comm      = MPI_COMM_NULL;
inline MPI_Comm registered_comm = MPI_COMM_NULL;

inline void ft_barrier(MPI_Comm comm) {
  int unused = -1;
  MPIX_Comm_agree(comm, &unused);
}

inline void safely_inject_failure() {
  ft_barrier(fenix_comm);
  if (0 == fenix::mpixx::comm_rank(MPI_COMM_WORLD)) raise(SIGTERM);
  MPI_Barrier(fenix_comm);
}

inline void init_registration_test() {
  int argc    = 0;
  char** argv = nullptr;
  MPI_Init(&argc, &argv);

  fenix::set_option(FENIX_RESUME_MODE, fenix::RETURN);
  fenix::set_option(FENIX_COMM_REGISTER_MODE, fenix::COMM_REGISTER_MANUAL);
  fenix::init({.out_comm = &fenix_comm, .spares = 0});

  MPI_Comm_dup(fenix_comm, &registered_comm);
  fenix::comm_register(registered_comm);
}

inline void finalize_registration_test() {
  if (registered_comm != MPI_COMM_NULL) MPI_Comm_free(&registered_comm);
  Fenix_Finalize();
  MPI_Finalize();
}

} // namespace comm_registration_test
