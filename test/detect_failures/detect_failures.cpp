#include <fenix.hpp>
#include <fenix/mpi_util.hpp>
#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>

const int kKillID = 1;

int n_failed() {
  int ret;
  MPI_Group global_failed;
  MPIX_Comm_get_failed(MPI_COMM_WORLD, &global_failed);
  MPI_Group_size(global_failed, &ret);
  MPI_Group_free(&global_failed);
  return ret;
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  fenix::set_option(FENIX_RESUME_MODE, fenix::RETURN);

  MPI_Comm res_world;
  fenix::init({.out_comm = &res_world, .spares = 1});

  int nranks, rank;
  MPI_Comm_size(res_world, &nranks);
  MPI_Comm_rank(res_world, &rank);
  assert(nranks > 1);

  if (n_failed() == 0) {
    MPI_Barrier(res_world);
    if (rank == 0) raise(SIGTERM);

    // Spin loop until we detect the failed rank.
    // Test succeeds if it returns instead of timing out.
    int ret;
    do {
      ret = fenix::detect_failures();
      fprintf(stderr, "Rank %d got return %d\n", rank, ret);
      usleep(10000);
    } while (ret != FENIX_ERROR_PROCESS_FAILURE);
  }

  if (n_failed() == 1) {
    MPI_Barrier(res_world);
    if (rank == 0) raise(SIGTERM);

    fenix::mlog::create(0, res_world, 1);
    fenix::mlog::activate(0);
    fenix::set_option(FENIX_MLOG_RECOVERY_MODE, fenix::INLINE);

    bool detected_failure = false;
    fenix::callback_register([&detected_failure](MPI_Comm, int) {
      detected_failure = true;
    });

    while (!detected_failure) {
      int ret = fenix::detect_failures();
      assert(ret == FENIX_SUCCESS);
      fprintf(stderr, "Rank %d got return %d from failure two\n", rank, ret);
      usleep(10000);
    }
  }

  Fenix_Finalize();

  MPI_Finalize();
}
