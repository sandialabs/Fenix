#include "comm_registration_common.hpp"

#include <mpi.h>

using namespace comm_registration_test;

int main(int argc, char** argv) {
  init_registration_test();

  MPI_Comm_disconnect(&registered_comm);

  safely_inject_failure();

  // Test passes if Fenix doesn't try to revoke the comm and break itself

  finalize_registration_test();
  return 0;
}
