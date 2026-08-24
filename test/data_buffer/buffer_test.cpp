#include "fenix/data/util/buffer.hpp"
using fenix::data::util::DataBuffer;
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

using namespace fenix;

// Helper to fill buffer with pattern
void fill_pattern(char* buf, size_t size, int pattern) {
  for (size_t i = 0; i < size; i++) {
    buf[i] = (char)((pattern + i) % 256);
  }
}

// Helper to verify pattern
bool verify_pattern(const char* buf, size_t size, int pattern) {
  for (size_t i = 0; i < size; i++) {
    if (buf[i] != (char)((pattern + i) % 256)) return false;
  }
  return true;
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0) fprintf(stderr, "Test 1: Basic construction and resize\n");
  {
    DataBuffer buf;
    if (buf.size() != 0) {
      fprintf(stderr, "FAIL: Initial size should be 0\n");
      return 1;
    }

    buf.resize(100);
    if (buf.size() != 100) {
      fprintf(stderr, "FAIL: Size should be 100 after resize\n");
      return 1;
    }

    fill_pattern(buf.data(), 100, 42);
    if (!verify_pattern(buf.data(), 100, 42)) {
      fprintf(stderr, "FAIL: Pattern mismatch after fill\n");
      return 1;
    }
  }

  if (rank == 0) fprintf(stderr, "Test 2: Constructor with initial size\n");
  {
    DataBuffer buf(50);
    if (buf.size() != 50) {
      fprintf(stderr, "FAIL: Size should be 50\n");
      return 1;
    }
  }

  if (rank == 0) fprintf(stderr, "Test 3: Grow buffer (preserves data)\n");
  {
    DataBuffer buf(100);
    fill_pattern(buf.data(), 100, 77);

    buf.resize(200);
    if (buf.size() != 200) {
      fprintf(stderr, "FAIL: Size should be 200 after grow\n");
      return 1;
    }

    // First 100 bytes should be preserved
    if (!verify_pattern(buf.data(), 100, 77)) {
      fprintf(stderr, "FAIL: Original data not preserved after grow\n");
      return 1;
    }
  }

  if (rank == 0) fprintf(stderr, "Test 4: Shrink buffer\n");
  {
    DataBuffer buf(200);
    fill_pattern(buf.data(), 200, 99);

    buf.resize(50);
    if (buf.size() != 50) {
      fprintf(stderr, "FAIL: Size should be 50 after shrink\n");
      return 1;
    }

    // First 50 bytes should be preserved
    if (!verify_pattern(buf.data(), 50, 99)) {
      fprintf(stderr, "FAIL: Data not preserved after shrink\n");
      return 1;
    }
  }

  if (rank == 0) fprintf(stderr, "Test 5: Reset (discards data)\n");
  {
    DataBuffer buf(100);
    fill_pattern(buf.data(), 100, 11);

    buf.reset(50);
    if (buf.size() != 50) {
      fprintf(stderr, "FAIL: Size should be 50 after reset\n");
      return 1;
    }

    // Data is discarded, just verify we can write new data
    fill_pattern(buf.data(), 50, 22);
    if (!verify_pattern(buf.data(), 50, 22)) {
      fprintf(stderr, "FAIL: Cannot write after reset\n");
      return 1;
    }
  }

  if (rank == 0) fprintf(stderr, "Test 6: Clear buffer\n");
  {
    DataBuffer buf(100);
    buf.clear();
    if (buf.size() != 0) {
      fprintf(stderr, "FAIL: Size should be 0 after clear\n");
      return 1;
    }
  }

  if (rank == 0) fprintf(stderr, "Test 7: Move construction\n");
  {
    DataBuffer buf1(100);
    fill_pattern(buf1.data(), 100, 33);
    char* original_ptr = buf1.data();

    DataBuffer buf2(std::move(buf1));
    if (buf2.size() != 100) {
      fprintf(stderr, "FAIL: Moved buffer should have size 100\n");
      return 1;
    }
    if (buf2.data() != original_ptr) {
      fprintf(stderr, "FAIL: Moved buffer should have same pointer\n");
      return 1;
    }
    if (!verify_pattern(buf2.data(), 100, 33)) {
      fprintf(stderr, "FAIL: Moved buffer data corrupted\n");
      return 1;
    }
  }

  if (rank == 0) fprintf(stderr, "Test 8: Move assignment\n");
  {
    DataBuffer buf1(100);
    fill_pattern(buf1.data(), 100, 44);
    char* original_ptr = buf1.data();

    DataBuffer buf2(50);
    buf2 = std::move(buf1);

    if (buf2.size() != 100) {
      fprintf(stderr, "FAIL: Move-assigned buffer should have size 100\n");
      return 1;
    }
    if (buf2.data() != original_ptr) {
      fprintf(stderr, "FAIL: Move-assigned buffer should have same pointer\n");
      return 1;
    }
    if (!verify_pattern(buf2.data(), 100, 44)) {
      fprintf(stderr, "FAIL: Move-assigned buffer data corrupted\n");
      return 1;
    }
  }

  if (rank == 0) fprintf(stderr, "Test 9: Take ownership of malloc'd buffer\n");
  {
    char* external_buf = (char*)malloc(150);
    fill_pattern(external_buf, 150, 55);

    DataBuffer buf;
    buf.take_ownership(external_buf, 150);

    if (buf.size() != 150) {
      fprintf(stderr, "FAIL: Owned buffer should have size 150\n");
      return 1;
    }
    if (buf.data() != external_buf) {
      fprintf(stderr, "FAIL: Owned buffer should have same pointer\n");
      return 1;
    }
    if (!verify_pattern(buf.data(), 150, 55)) {
      fprintf(stderr, "FAIL: Owned buffer data corrupted\n");
      return 1;
    }

    // Destructor will free the buffer
  }

  if (rank == 0) fprintf(stderr, "Test 10: Take ownership of mmapped buffer\n");
  {
    size_t size    = 256;
    char* mmap_buf = (char*)mmap(
      nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
    );
    if (mmap_buf == MAP_FAILED) {
      fprintf(stderr, "FAIL: mmap failed\n");
      return 1;
    }

    fill_pattern(mmap_buf, size, 66);

    DataBuffer buf;
    buf.take_ownership_mmapped(mmap_buf, size);

    if (buf.size() != size) {
      fprintf(stderr, "FAIL: Mmapped buffer should have size %zu\n", size);
      return 1;
    }
    if (!verify_pattern(buf.data(), size, 66)) {
      fprintf(stderr, "FAIL: Mmapped buffer data corrupted\n");
      return 1;
    }

    // Destructor will munmap the buffer
  }

  if (rank == 0)
    fprintf(stderr, "Test 11: Resize mmapped buffer (forces malloc copy)\n");
  {
    size_t size    = 128;
    char* mmap_buf = (char*)mmap(
      nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
    );
    if (mmap_buf == MAP_FAILED) {
      fprintf(stderr, "FAIL: mmap failed\n");
      return 1;
    }

    fill_pattern(mmap_buf, size, 77);

    DataBuffer buf;
    buf.take_ownership_mmapped(mmap_buf, size);

    // Resize to larger size - should force malloc and copy
    buf.resize(256);
    if (buf.size() != 256) {
      fprintf(stderr, "FAIL: Resized mmapped buffer should have size 256\n");
      return 1;
    }

    // Original data should be preserved
    if (!verify_pattern(buf.data(), size, 77)) {
      fprintf(stderr, "FAIL: Data not preserved after mmapped buffer resize\n");
      return 1;
    }
  }

  if (rank == 0) fprintf(stderr, "Test 12: Resize from zero size\n");
  {
    DataBuffer buf;
    buf.resize(0); // Already zero, but test the path
    buf.resize(100);
    if (buf.size() != 100) {
      fprintf(stderr, "FAIL: Should be able to resize from zero\n");
      return 1;
    }
  }

  if (rank == 0) fprintf(stderr, "Test 13: Multiple resize operations\n");
  {
    DataBuffer buf(10);
    fill_pattern(buf.data(), 10, 88);

    // Grow
    buf.resize(50);
    if (!verify_pattern(buf.data(), 10, 88)) {
      fprintf(stderr, "FAIL: Data lost after first grow\n");
      return 1;
    }

    // Grow more
    buf.resize(200);
    if (!verify_pattern(buf.data(), 10, 88)) {
      fprintf(stderr, "FAIL: Data lost after second grow\n");
      return 1;
    }

    // Shrink
    buf.resize(30);
    if (buf.size() != 30) {
      fprintf(stderr, "FAIL: Size should be 30 after shrink\n");
      return 1;
    }
    if (!verify_pattern(buf.data(), 10, 88)) {
      fprintf(stderr, "FAIL: Data lost after shrink\n");
      return 1;
    }
  }

  if (rank == 0) fprintf(stderr, "All DataBuffer tests passed!\n");

  MPI_Finalize();
  return 0;
}
