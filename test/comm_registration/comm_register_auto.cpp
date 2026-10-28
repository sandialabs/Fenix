#include "comm_registration_common.hpp"

#include <cstring>
#include <cstdio>
#include <mpi.h>

using namespace comm_registration_test;

#ifndef OMPI_MAJOR_VERSION
#define OMPI_MAJOR_VERSION 0
#endif

int main(int argc, char** argv) {
  if (OMPI_MAJOR_VERSION < 6) {
    printf("Skipping test for OMPI_MAJOR_VERSION=%d\n", OMPI_MAJOR_VERSION);
    return 0;
  } else {
    printf("Conducting test for OMPI_MAJOR_VERSION=%d\n", OMPI_MAJOR_VERSION);
  }

  init_registration_test();

  fenix::set_option(FENIX_COMM_REGISTER_MODE, fenix::COMM_REGISTER_AUTO);

  MPI_Comm duplicated_comm = MPI_COMM_NULL;
  MPI_Comm_dup(registered_comm, &duplicated_comm);

  safely_inject_failure();

  int revoked = 0;
  MPIX_Comm_is_revoked(duplicated_comm, &revoked);
  fenix_require(revoked, "Duplicated comm should be revoked after recovery");

  MPI_Comm_free(&duplicated_comm);
  finalize_registration_test();
  return 0;
}
