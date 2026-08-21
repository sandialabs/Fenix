#include <fenix.hpp>
#include <fenix/data/util/mstream.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

constexpr int my_group = 0;
constexpr int my_member = 0;

using fenix::DataSubset;
using namespace fenix::data;

std::vector<int> data;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  // Use only 2 ranks to avoid IMR partner issues
  if (num_ranks != 2) {
    if (rank == 0) fprintf(stderr, "SKIP: This test requires exactly 2 ranks\n");
    Fenix_Finalize();
    MPI_Finalize();
    return 0;
  }

  // Modify chunk sizes to smaller values for testing
  if (rank == 0) fprintf(stderr, "Setting small chunk sizes for testing\n");
  fenix::data::util::detail::OMmapStreamBuf::target_claim_chunk_size = 40 * 1024; // 40KB
  fenix::data::util::detail::OMmapStreamBuf::target_write_chunk_size = 4 * 1024;  // 4KB

  group_create(my_group, {.depth = 1});

  if (rank == 0) fprintf(stderr, "Test: Seeking and writing past chunk boundaries\n");

  // Create data that will span multiple write chunks (3KB per rank)
  data.resize(768 + rank); // 768-769 ints = 3072-3076 bytes
  for (int i = 0; i < data.size(); i++) data[i] = rank * 1000 + i;

  // Create resizeable member with iostream serializer that uses extensive seeking
  member_create(
    my_group, my_member, nullptr, FENIX_RESIZEABLE, MPI_INT,
    [&](std::iostream& strm, int dir, void* b, int offset, int count) {
      fenix_require(offset == 0 && b == nullptr);

      if (dir == FENIX_SERIALIZE) {
        fenix_require(count == FENIX_RESIZEABLE);

        // Write size header
        int size = data.size();
        strm.write((char*)&size, sizeof(int));

        if (rank == 0) fprintf(stderr, "Writing %d ints (%zu bytes) with seeking\n",
                               size, size * sizeof(int));

        // Test 1: Write data in chunks, seeking between them
        int quarter = size / 4;

        // Write first quarter
        strm.write((char*)data.data(), sizeof(int) * quarter);
        auto pos1 = strm.tellp();
        if (rank == 0) fprintf(stderr, "After first quarter: pos=%ld\n", (long)pos1);

        // Seek forward by 1 write_chunk (4KB) - should trigger overflow/grow
        strm.seekp(4096, std::ios_base::cur);
        auto pos2 = strm.tellp();
        if (rank == 0) fprintf(stderr, "After seeking +4KB: pos=%ld\n", (long)pos2);

        // Write some marker data
        int marker = 0xCAFEBABE;
        strm.write((char*)&marker, sizeof(int));

        // Test 2: Seek ahead multiple write_chunks at once (skip 3 chunks = 12KB)
        strm.seekp(12288, std::ios_base::cur);
        auto pos3 = strm.tellp();
        if (rank == 0) fprintf(stderr, "After seeking +12KB: pos=%ld\n", (long)pos3);

        // Write another marker
        int marker2 = 0xDEADBEEF;
        strm.write((char*)&marker2, sizeof(int));

        // Test 3: Seek back to original position and continue writing data
        strm.seekp(pos1);
        strm.write((char*)(data.data() + quarter), sizeof(int) * (size - quarter));
        auto pos4 = strm.tellp();
        if (rank == 0) fprintf(stderr, "After writing remaining data: pos=%ld\n", (long)pos4);

        // Test 4: Seek to end (std::ios_base::end)
        strm.seekp(0, std::ios_base::end);
        auto pos5 = strm.tellp();
        if (rank == 0) fprintf(stderr, "After seekp(0, end): pos=%ld\n", (long)pos5);

      } else {
        // Deserialize
        int size;
        strm.read((char*)&size, sizeof(int));
        data.resize(size);
        strm.read((char*)data.data(), sizeof(int) * size);

        // Test seekg
        strm.seekg(0, std::ios_base::end);
        strm.seekg(-sizeof(int), std::ios_base::cur);
      }
    }
  );

  // Commit the staged data
  if (rank == 0) fprintf(stderr, "Staging and committing with seeking serializer\n");
  member_stage(my_group, my_member, SUBSET_FULL);
  member_storev(my_group, my_member, SUBSET_PRESTAGED);
  commit(my_group);

  // Now restore and verify the data
  if (rank == 0) fprintf(stderr, "Restoring data to verify seeking worked correctly\n");

  // Save original data for comparison
  std::vector<int> original_data = data;

  // Clear data
  data.clear();
  data.resize(2000, -1);

  // Restore from the checkpoint
  member_restore(
    my_group, my_member, FENIX_DATA_RESTORE_INPLACE,
    FENIX_DATA_RESTORE_FULL, FENIX_DATA_SNAPSHOT_LATEST
  );

  // Verify restored data matches original
  if (data.size() != original_data.size()) {
    fprintf(stderr, "Rank %d: Size mismatch! Expected %zu, got %zu\n",
            rank, original_data.size(), data.size());
    MPI_Abort(res_comm, 1);
  }

  for (size_t i = 0; i < data.size(); i++) {
    if (data[i] != original_data[i]) {
      fprintf(stderr, "Rank %d: Data mismatch at index %zu! Expected %d, got %d\n",
              rank, i, original_data[i], data[i]);
      MPI_Abort(res_comm, 1);
    }
  }

  if (rank == 0) fprintf(stderr, "All seeking tests passed - data verified!\n");
  if (rank == 0) fprintf(stderr, "Successfully wrote past first write_chunk and skipped multiple chunks\n");

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
