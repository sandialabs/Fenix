#include "comm_registration_common.hpp"

#include <mpi.h>

using namespace comm_registration_test;

int main(int argc, char** argv) {
  init_registration_test();

  fenix::callback_register(
    [](MPI_Comm, int) {
      int revoked;
      MPIX_Comm_is_revoked(registered_comm, &revoked);
      fenix_require(
        revoked,
        "Registered comm should be revoked before PRE_RECOVERY callback!"
      );
    },
    fenix::PRE_RECOVERY
  );

  safely_inject_failure();

  int revoked;
  MPIX_Comm_is_revoked(registered_comm, &revoked);
  fenix_require(revoked, "Registered comm should be revoked after recovery");

  finalize_registration_test();
  return 0;
}
