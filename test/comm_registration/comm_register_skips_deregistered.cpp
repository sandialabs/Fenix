#include "comm_registration_common.hpp"

#include <mpi.h>

using namespace comm_registration_test;

int main(int argc, char** argv) {
  init_registration_test();

  fenix::comm_deregister(registered_comm);

  safely_inject_failure();

  int revoked;
  MPIX_Comm_is_revoked(registered_comm, &revoked);
  fenix_require(!revoked, "Deregistered comm should not be revoked by Fenix");

  finalize_registration_test();
  return 0;
}
