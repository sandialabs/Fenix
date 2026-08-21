#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

using namespace fenix::data;

constexpr int my_group = 0;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  // Use only 2 ranks to avoid IMR partner issues
  if (num_ranks != 2) {
    if (rank == 0)
      fprintf(stderr, "SKIP: This test requires exactly 2 ranks\n");
    Fenix_Finalize();
    MPI_Finalize();
    return 0;
  }

  group_create(my_group, {.depth = 1});

  //===========================================================================
  // Test 1: Reduce count before staging, verify only reduced count is stored
  //===========================================================================
  if (rank == 0)
    fprintf(stderr, "Test 1: Reduce count from 100 to 50 before staging\n");

  std::vector<int> data(100);
  for (int i = 0; i < 100; i++) data[i] = rank * 1000 + i;

  member_create(my_group, 1, data.data(), 100, MPI_INT);

  // Reduce count to 50 BEFORE staging
  int new_count = 50;
  int flag      = 0;
  member_attr_set(
    my_group, 1, FENIX_DATA_MEMBER_ATTRIBUTE_COUNT, &new_count, &flag
  );

  // Checkpoint with reduced count
  checkpoint(my_group, SUBSET_FULL);

  // Corrupt all 100 elements
  for (int i = 0; i < 100; i++) data[i] = -999;

  // Load from checkpoint - should only restore 50 elements
  if (rank == 0)
    fprintf(
      stderr, "  Loading checkpoint, expecting only 50 elements restored\n"
    );
  member_restore(
    my_group, 1, FENIX_DATA_RESTORE_INPLACE, FENIX_DATA_RESTORE_FULL,
    FENIX_DATA_SNAPSHOT_LATEST
  );

  // Verify first 50 elements were restored correctly
  for (int i = 0; i < 50; i++) {
    if (data[i] != rank * 1000 + i) {
      fprintf(
        stderr, "Rank %d: ERROR in element %d, expected %d, got %d\n", rank, i,
        rank * 1000 + i, data[i]
      );
      MPI_Abort(res_comm, 1);
    }
  }

  // Verify last 50 elements were NOT restored (should still be -999)
  for (int i = 50; i < 100; i++) {
    if (data[i] != -999) {
      fprintf(
        stderr,
        "Rank %d: ERROR - element %d was restored but shouldn't have been (got "
        "%d)\n",
        rank, i, data[i]
      );
      MPI_Abort(res_comm, 1);
    }
  }

  if (rank == 0) fprintf(stderr, "  ✓ Only 50 elements restored as expected\n");

  member_delete(my_group, 1);

  //===========================================================================
  // Test 2: Change datatype before staging, verify size is recalculated
  //===========================================================================
  if (rank == 0)
    fprintf(stderr, "Test 2: Change from MPI_2INT to MPI_INT before staging\n");

  // Create member with MPI_2INT (pairs of ints) = 50 pairs = 100 ints worth
  std::vector<int> pair_data(100);
  for (int i = 0; i < 100; i++) pair_data[i] = rank * 1000 + i;

  member_create(my_group, 2, pair_data.data(), 50, MPI_2INT);

  // Change to MPI_INT BEFORE any staging
  // Was 50 MPI_2INTs (count=50, 8 bytes each = 400 bytes total)
  // Now 50 MPI_INTs (count=50, 4 bytes each = 200 bytes total)
  // Fenix should automatically recalculate user_data size
  // This means only first 50 ints of the 100-int buffer will be checkpointed
  MPI_Datatype new_dtype = MPI_INT;
  member_attr_set(
    my_group, 2, FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE, &new_dtype, &flag
  );

  // Checkpoint with MPI_INT and count=50
  checkpoint(my_group, SUBSET_FULL);

  // Corrupt all data
  for (int i = 0; i < 100; i++) pair_data[i] = -888;

  // Load from checkpoint - should restore only first 50 ints
  // Note: There will be a warning about earlier timestamps being unrecoverable
  // for member 2 - this is expected because member 2 didn't exist when we
  // checkpointed member 1 in Test 1.
  if (rank == 0)
    fprintf(stderr, "  Loading checkpoint, expecting 50 MPI_INTs restored\n");
  member_restore(
    my_group, 2, FENIX_DATA_RESTORE_INPLACE, FENIX_DATA_RESTORE_FULL,
    FENIX_DATA_SNAPSHOT_LATEST
  );

  // Verify first 50 ints restored
  for (int i = 0; i < 50; i++) {
    if (pair_data[i] != rank * 1000 + i) {
      fprintf(
        stderr, "Rank %d: ERROR in element %d, expected %d, got %d\n", rank, i,
        rank * 1000 + i, pair_data[i]
      );
      MPI_Abort(res_comm, 1);
    }
  }

  // Verify last 50 were not restored (should still be -888)
  for (int i = 50; i < 100; i++) {
    if (pair_data[i] != -888) {
      fprintf(
        stderr,
        "Rank %d: ERROR - element %d was restored but shouldn't (got %d)\n",
        rank, i, pair_data[i]
      );
      MPI_Abort(res_comm, 1);
    }
  }

  if (rank == 0) fprintf(stderr, "  ✓ Datatype change worked correctly\n");

  member_delete(my_group, 2);

  //===========================================================================
  // Test 3: Verify BUFFER attribute can be changed after staging
  //===========================================================================
  if (rank == 0)
    fprintf(stderr, "Test 3: Verify buffer pointer can change after staging\n");

  std::vector<int> buf1(50), buf2(50);
  for (int i = 0; i < 50; i++) {
    buf1[i] = rank * 100 + i;
    buf2[i] = rank * 200 + i;
  }

  member_create(my_group, 3, buf1.data(), 50, MPI_INT);
  checkpoint(my_group, SUBSET_FULL);

  // Change buffer pointer after staging - this should be allowed
  member_attr_set(
    my_group, 3, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, buf2.data(), &flag
  );

  // Stage again with new buffer
  checkpoint(my_group, SUBSET_FULL);

  // Clear buf2 and restore
  for (int i = 0; i < 50; i++) buf2[i] = -777;

  member_restore(
    my_group, 3, FENIX_DATA_RESTORE_INPLACE, FENIX_DATA_RESTORE_FULL,
    FENIX_DATA_SNAPSHOT_LATEST
  );

  // Verify buf2 has the data from the second checkpoint
  for (int i = 0; i < 50; i++) {
    if (buf2[i] != rank * 200 + i) {
      fprintf(
        stderr, "Rank %d: ERROR after buffer change, element %d = %d\n", rank,
        i, buf2[i]
      );
      MPI_Abort(res_comm, 1);
    }
  }

  if (rank == 0)
    fprintf(stderr, "  ✓ Buffer pointer change worked correctly\n");

  member_delete(my_group, 3);

  //===========================================================================
  // Test 4: Verify count change rejected after staging
  //===========================================================================
  if (rank == 0)
    fprintf(
      stderr,
      "Test 4: Verify count change to different value rejected after staging\n"
    );

  std::vector<int> test_data(100);
  for (int i = 0; i < 100; i++) test_data[i] = i;

  member_create(my_group, 4, test_data.data(), 100, MPI_INT);
  checkpoint(my_group, SUBSET_FULL);

  // Attempt to change count to a DIFFERENT value after staging - should throw
  new_count             = 50;
  bool caught_exception = false;
  try {
    member_attr_set(
      my_group, 4, FENIX_DATA_MEMBER_ATTRIBUTE_COUNT, &new_count, &flag
    );
  } catch (const fenix::RuntimeException& e) {
    caught_exception = true;
  }

  if (!caught_exception) {
    if (rank == 0)
      fprintf(stderr, "  ✗ ERROR: Count change should have thrown!\n");
    MPI_Abort(res_comm, 1);
  }

  if (rank == 0)
    fprintf(stderr, "  ✓ Count change correctly rejected after staging\n");

  // But setting to the SAME value should be allowed (for member_define)
  new_count = 100;
  try {
    member_attr_set(
      my_group, 4, FENIX_DATA_MEMBER_ATTRIBUTE_COUNT, &new_count, &flag
    );
    if (rank == 0) fprintf(stderr, "  ✓ Setting count to same value allowed\n");
  } catch (const fenix::RuntimeException& e) {
    if (rank == 0)
      fprintf(
        stderr, "  ✗ ERROR: Setting count to same value should be allowed!\n"
      );
    MPI_Abort(res_comm, 1);
  }

  member_delete(my_group, 4);

  //===========================================================================
  // Test 5: Verify datatype change rejected after staging
  //===========================================================================
  if (rank == 0)
    fprintf(
      stderr,
      "Test 5: Verify datatype change to different value rejected after "
      "staging\n"
    );

  member_create(my_group, 5, test_data.data(), 100, MPI_INT);
  checkpoint(my_group, SUBSET_FULL);

  // Attempt to change datatype to a DIFFERENT value after staging - should
  // throw
  MPI_Datatype dtype_double = MPI_DOUBLE;
  caught_exception          = false;
  try {
    member_attr_set(
      my_group, 5, FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE, &dtype_double, &flag
    );
  } catch (const fenix::RuntimeException& e) {
    caught_exception = true;
  }

  if (!caught_exception) {
    if (rank == 0)
      fprintf(stderr, "  ✗ ERROR: Datatype change should have thrown!\n");
    MPI_Abort(res_comm, 1);
  }

  if (rank == 0)
    fprintf(stderr, "  ✓ Datatype change correctly rejected after staging\n");

  // But setting to the SAME datatype should be allowed (for member_define)
  MPI_Datatype dtype_int = MPI_INT;
  try {
    member_attr_set(
      my_group, 5, FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE, &dtype_int, &flag
    );
    if (rank == 0)
      fprintf(stderr, "  ✓ Setting datatype to same value allowed\n");
  } catch (const fenix::RuntimeException& e) {
    if (rank == 0)
      fprintf(
        stderr, "  ✗ ERROR: Setting datatype to same value should be allowed!\n"
      );
    MPI_Abort(res_comm, 1);
  }

  member_delete(my_group, 5);

  if (rank == 0) fprintf(stderr, "\nAll member attribute tests passed!\n");

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
