#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

using namespace fenix::data;

constexpr int my_group  = 0;
constexpr int my_member = 1;

std::vector<int> dynamic_data;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  if (num_ranks != 2) {
    if (rank == 0)
      fprintf(stderr, "SKIP: This test requires exactly 2 ranks\n");
    Fenix_Finalize();
    MPI_Finalize();
    return 0;
  }

  group_create(my_group, {.depth = 2});

  if (rank == 0)
    fprintf(stderr, "Test: member_define with custom serializer\n");

  // Test 1: Define member with serializer and checkpoint
  if (rank == 0) fprintf(stderr, "  Creating member with serializer\n");
  dynamic_data = {rank * 10, rank * 10 + 1, rank * 10 + 2};

  member_define(
    my_group, my_member, nullptr, FENIX_RESIZEABLE, MPI_INT,
    [&](std::iostream& strm, int dir, void* b, int offset, int count) {
      fenix_require(offset == 0 && b == nullptr);
      if (dir == FENIX_SERIALIZE) {
        fenix_require(count == FENIX_RESIZEABLE);
        int size = dynamic_data.size();
        strm.write((char*)&size, sizeof(int));
        strm.write((char*)dynamic_data.data(), sizeof(int) * size);
      } else {
        int size;
        strm.read((char*)&size, sizeof(int));
        dynamic_data.resize(size);
        strm.read((char*)dynamic_data.data(), sizeof(int) * size);
      }
    }
  );

  checkpoint(my_group, SUBSET_FULL);

  // Test 2: Redefine member with different data and checkpoint again
  if (rank == 0) fprintf(stderr, "  Redefining member with new data\n");
  dynamic_data = {rank * 100, rank * 100 + 1, rank * 100 + 2, rank * 100 + 3};

  member_define(
    my_group, my_member, nullptr, FENIX_RESIZEABLE, MPI_INT,
    [&](std::iostream& strm, int dir, void* b, int offset, int count) {
      fenix_require(offset == 0 && b == nullptr);
      if (dir == FENIX_SERIALIZE) {
        fenix_require(count == FENIX_RESIZEABLE);
        int size = dynamic_data.size();
        strm.write((char*)&size, sizeof(int));
        strm.write((char*)dynamic_data.data(), sizeof(int) * size);
      } else {
        int size;
        strm.read((char*)&size, sizeof(int));
        dynamic_data.resize(size);
        strm.read((char*)dynamic_data.data(), sizeof(int) * size);
      }
    }
  );

  checkpoint(my_group, SUBSET_FULL);

  // Test 3: Restore from first checkpoint (timestamp 0)
  if (rank == 0) fprintf(stderr, "  Restoring from timestamp 0\n");
  std::vector<int> expected_first = {rank * 10, rank * 10 + 1, rank * 10 + 2};
  dynamic_data.clear();

  member_restore(
    my_group, my_member, FENIX_DATA_RESTORE_INPLACE, FENIX_DATA_RESTORE_FULL, 0
  );

  if (dynamic_data.size() != 3) {
    fprintf(
      stderr, "Rank %d: ERROR restoring timestamp 0, size=%zu (expected 3)\n",
      rank, dynamic_data.size()
    );
    MPI_Abort(res_comm, 1);
  }

  for (size_t i = 0; i < dynamic_data.size(); i++) {
    if (dynamic_data[i] != expected_first[i]) {
      fprintf(
        stderr, "Rank %d: ERROR at [%zu], got %d, expected %d\n", rank, i,
        dynamic_data[i], expected_first[i]
      );
      MPI_Abort(res_comm, 1);
    }
  }

  if (rank == 0) fprintf(stderr, "  ✓ Timestamp 0 restored correctly\n");

  // Test 4: Restore from second checkpoint (timestamp 1)
  if (rank == 0) fprintf(stderr, "  Restoring from timestamp 1\n");
  std::vector<int> expected_second = {
    rank * 100, rank * 100 + 1, rank * 100 + 2, rank * 100 + 3
  };
  dynamic_data.clear();

  member_restore(
    my_group, my_member, FENIX_DATA_RESTORE_INPLACE, FENIX_DATA_RESTORE_FULL, 1
  );

  if (dynamic_data.size() != 4) {
    fprintf(
      stderr, "Rank %d: ERROR restoring timestamp 1, size=%zu (expected 4)\n",
      rank, dynamic_data.size()
    );
    MPI_Abort(res_comm, 1);
  }

  for (size_t i = 0; i < dynamic_data.size(); i++) {
    if (dynamic_data[i] != expected_second[i]) {
      fprintf(
        stderr, "Rank %d: ERROR at [%zu], got %d, expected %d\n", rank, i,
        dynamic_data[i], expected_second[i]
      );
      MPI_Abort(res_comm, 1);
    }
  }

  if (rank == 0) fprintf(stderr, "  ✓ Timestamp 1 restored correctly\n");

  if (rank == 0)
    fprintf(stderr, "All member_define serializer tests passed!\n");

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
