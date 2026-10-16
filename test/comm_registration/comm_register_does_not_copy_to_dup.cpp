#include "comm_registration_common.hpp"

#include <mpi.h>

using namespace comm_registration_test;

int main(int argc, char** argv) {
  init_registration_test();

  MPI_Comm duplicated_comm = MPI_COMM_NULL;
  MPI_Comm_dup(registered_comm, &duplicated_comm);

  safely_inject_failure();

  int revoked;
  MPIX_Comm_is_revoked(registered_comm, &revoked);
  fenix_require(revoked, "Registered comm should be revoked after recovery");

  MPIX_Comm_is_revoked(duplicated_comm, &revoked);
  fenix_require(!revoked, "Duplicated comm should not inherit registration");

  MPI_Comm_free(&duplicated_comm);
  finalize_registration_test();
  return 0;
}
