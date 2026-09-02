/*
//@HEADER
// ************************************************************************
//
//
//            _|_|_|_|  _|_|_|_|  _|      _|  _|_|_|  _|      _|
//            _|        _|        _|_|    _|    _|      _|  _|
//            _|_|_|    _|_|_|    _|  _|  _|    _|        _|
//            _|        _|        _|    _|_|    _|      _|  _|
//            _|        _|_|_|_|  _|      _|  _|_|_|  _|      _|
//
//
//
//
// Copyright (C) 2016 Rutgers University and Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY RUTGERS UNIVERSITY and SANDIA CORPORATION
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RUTGERS
// UNIVERISY, SANDIA CORPORATION OR THE CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author Marc Gamell, Eric Valenzuela, Keita Teranishi, Manish Parashar,
//        Michael Heroux, and Matthew Whitlock
//
// Questions? Contact Matthew Whitlock (mwhitlo@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include <fenix.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cstring>
#include <algorithm>

#include "fenix/data/subset.hpp"
#include "fenix/mpixx/datatype.hpp"
#include "subset_common.hpp"

using namespace fenix;

// Sentinel values to detect buffer overruns
constexpr int SENTINEL_BEFORE = 0xDEADBEEF;
constexpr int SENTINEL_AFTER = 0xCAFEBABE;

// Test helper: verify only expected data was written
template<typename T>
bool test_with_sentinels(
  const DataSubset& subset,
  MPI_Datatype base_type,
  const std::vector<T>& src_data,
  int partner_rank,
  int tag
) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  size_t expected_count = subset.count(subset.end());
  if (expected_count == 0) return true;

  std::string subset_str = subset.str();

  if (rank == 0) {
    auto dt = subset.to_datatype(mpixx::DatatypeRef(base_type));
    MPI_Send(const_cast<T*>(src_data.data()), 1, dt, partner_rank, tag, MPI_COMM_WORLD);
  } else if (rank == partner_rank) {
    // Allocate receive buffer same size as source, use datatype to place data correctly
    int before_sentinel = SENTINEL_BEFORE;
    std::vector<T> recv_data(src_data.size(), static_cast<T>(-1));
    int after_sentinel = SENTINEL_AFTER;

    auto dt = subset.to_datatype(mpixx::DatatypeRef(base_type));
    MPI_Recv(recv_data.data(), 1, dt, 0, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Check sentinels weren't overwritten
    fenix_require(
      before_sentinel == SENTINEL_BEFORE,
      "Buffer underrun for subset %s: sentinel before data was overwritten", subset_str.c_str()
    );
    fenix_require(
      after_sentinel == SENTINEL_AFTER,
      "Buffer overrun for subset %s: sentinel after data was overwritten", subset_str.c_str()
    );

    // Verify received data matches expected subset elements
    for (size_t i = 0; i < src_data.size(); i++) {
      if (subset.includes(i)) {
        fenix_require(
          std::memcmp(&recv_data[i], &src_data[i], sizeof(T)) == 0,
          "Data mismatch for subset %s at index %zu", subset_str.c_str(), i
        );
      } else {
        // Verify non-subset elements weren't touched
        T expected_untouched = static_cast<T>(-1);
        fenix_require(
          std::memcmp(&recv_data[i], &expected_untouched, sizeof(T)) == 0,
          "Non-subset index %zu was modified for subset %s", i, subset_str.c_str()
        );
      }
    }
  }

  return true;
}

// Test with builtin types of various sizes
template<typename T>
bool test_builtin_type(const DataSubset& subset, MPI_Datatype mpi_type, int tag) {
  // Array must be large enough to hold all indices referenced by the subset
  size_t array_size = subset.empty() ? 100 : (subset.end() + 1);

  std::vector<T> src_data(array_size);
  for (size_t i = 0; i < array_size; i++) {
    src_data[i] = static_cast<T>(i * 13 + 7);
  }

  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  fenix_require(size >= 2, "Need at least 2 ranks");

  return test_with_sentinels(subset, mpi_type, src_data, 1, tag);
}

// Test with custom struct datatype
struct CustomStruct {
  int32_t a;
  double b;
  int16_t c;

  bool operator==(const CustomStruct& other) const {
    return a == other.a && b == other.b && c == other.c;
  }
  bool operator!=(const CustomStruct& other) const {
    return !(*this == other);
  }
};

bool test_custom_struct_type(const DataSubset& subset, int tag) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  size_t expected_count = subset.count(subset.end());
  if (expected_count == 0) return true;

  size_t array_size = subset.empty() ? 100 : (subset.end() + 1);
  std::vector<CustomStruct> src_data(array_size);
  for (size_t i = 0; i < array_size; i++) {
    src_data[i] = {static_cast<int32_t>(i * 13 + 7), static_cast<double>(i * 13 + 7), static_cast<int16_t>(i * 13 + 7)};
  }

  // Create MPI struct datatype
  int blocklengths[] = {1, 1, 1};
  MPI_Aint displacements[] = {
    offsetof(CustomStruct, a),
    offsetof(CustomStruct, b),
    offsetof(CustomStruct, c)
  };
  MPI_Datatype types[] = {MPI_INT32_T, MPI_DOUBLE, MPI_INT16_T};

  MPI_Datatype custom_type;
  MPI_Type_create_struct(3, blocklengths, displacements, types, &custom_type);
  MPI_Type_commit(&custom_type);

  std::string subset_str = subset.str();

  if (rank == 0) {
    auto dt = subset.to_datatype(mpixx::DatatypeRef(custom_type));
    MPI_Send(src_data.data(), 1, dt, 1, tag, MPI_COMM_WORLD);
  } else if (rank == 1) {
    CustomStruct sentinel = {-1, -1.0, -1};
    std::vector<CustomStruct> recv_data(array_size, sentinel);

    auto dt = subset.to_datatype(mpixx::DatatypeRef(custom_type));
    MPI_Recv(recv_data.data(), 1, dt, 0, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Verify received data matches expected subset elements
    for (size_t i = 0; i < array_size; i++) {
      if (subset.includes(i)) {
        fenix_require(
          recv_data[i] == src_data[i],
          "Data mismatch for subset %s at index %zu", subset_str.c_str(), i
        );
      } else {
        // Verify non-subset elements weren't touched
        fenix_require(
          recv_data[i] == sentinel,
          "Non-subset index %zu was modified for subset %s", i, subset_str.c_str()
        );
      }
    }
  }

  MPI_Type_free(&custom_type);
  return true;
}

// Test with MPI vector datatype (resized to have extent != size)
bool test_vector_datatype(const DataSubset& subset, int tag) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  size_t expected_count = subset.count(subset.end());
  if (expected_count == 0) return true;

  size_t array_size = subset.empty() ? 100 : (subset.end() + 1);

  // Each element is 3 contiguous ints, but with padding to make extent = 5 ints
  // Allocate with spacing of 5 ints per element
  std::vector<int> src_data(array_size * 5);
  for (size_t i = 0; i < array_size; i++) {
    // Only fill the first 3 ints of each 5-int block
    src_data[i * 5 + 0] = static_cast<int>(i * 13 + 7);
    src_data[i * 5 + 1] = static_cast<int>(i * 13 + 8);
    src_data[i * 5 + 2] = static_cast<int>(i * 13 + 9);
    // Leave indices i*5+3 and i*5+4 as zero (padding)
  }

  // Create a type that contains 3 ints but has extent of 5 ints (to match array layout)
  MPI_Datatype contiguous_type;
  MPI_Type_contiguous(3, MPI_INT, &contiguous_type);

  MPI_Datatype vec_type;
  MPI_Type_create_resized(contiguous_type, 0, 5 * sizeof(int), &vec_type);
  MPI_Type_commit(&vec_type);
  MPI_Type_free(&contiguous_type);

  std::string subset_str = subset.str();

  if (rank == 0) {
    auto dt = subset.to_datatype(mpixx::DatatypeRef(vec_type));
    MPI_Send(src_data.data(), 1, dt, 1, tag, MPI_COMM_WORLD);
  } else if (rank == 1) {
    std::vector<int> recv_data(array_size * 5, -1);

    auto dt = subset.to_datatype(mpixx::DatatypeRef(vec_type));
    MPI_Recv(recv_data.data(), 1, dt, 0, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Verify received data matches expected subset elements
    // Each element consists of 3 ints at positions [i*5, i*5+1, i*5+2]
    // Positions [i*5+3, i*5+4] are padding and should remain -1
    for (size_t i = 0; i < array_size; i++) {
      if (subset.includes(i)) {
        // Check the 3 data ints
        for (int j = 0; j < 3; j++) {
          fenix_require(
            recv_data[i * 5 + j] == src_data[i * 5 + j],
            "Data mismatch for subset %s at element %zu, offset %d", subset_str.c_str(), i, j
          );
        }
        // Padding should still be -1
        for (int j = 3; j < 5; j++) {
          fenix_require(
            recv_data[i * 5 + j] == -1,
            "Padding mismatch for subset %s at element %zu, offset %d", subset_str.c_str(), i, j
          );
        }
      } else {
        // All 5 ints should be untouched
        for (int j = 0; j < 5; j++) {
          fenix_require(
            recv_data[i * 5 + j] == -1,
            "Non-subset element %zu was modified at offset %d for subset %s", i, j, subset_str.c_str()
          );
        }
      }
    }
  }

  MPI_Type_free(&vec_type);
  return true;
}

// Test empty subset
bool test_empty_subset() {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  DataSubset empty_subset;
  fenix_require(empty_subset.empty(), "Subset should be empty");

  if (rank == 0) {
    auto dt = empty_subset.to_datatype(mpixx::DatatypeRef(MPI_INT));

    std::vector<int> dummy(100, 42);
    MPI_Send(dummy.data(), 1, dt, 1, 0, MPI_COMM_WORLD);
  } else if (rank == 1) {
    std::vector<int> recv(10, -1);
    MPI_Recv(recv.data(), 0, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Verify nothing was written
    for (int val : recv) {
      fenix_require(val == -1, "Empty subset should not write any data");
    }
  }

  return true;
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (size < 2) {
    if (rank == 0) {
      fprintf(stderr, "ERROR: This test requires at least 2 MPI ranks\n");
    }
    MPI_Finalize();
    return 1;
  }

  int test_count = 0;
  int pass_count = 0;

  auto all_subsets = get_expanded_subsets();

  // Filter to only bounded subsets
  std::vector<DataSubset> subsets;
  for (const auto& s : all_subsets) {
    if (s.is_bounded()) {
      subsets.push_back(s);
    }
  }

  if (rank == 0) {
    printf("Testing to_datatype() with %zu bounded subsets (out of %zu total)\n",
           subsets.size(), all_subsets.size());
  }

  // Test 1: Empty subset (special case, doesn't need to be bounded)
  test_count++;
  try {
    if (test_empty_subset()) {
      pass_count++;
    } else if (rank == 0) {
      printf("FAILED: empty subset test\n");
    }
  } catch (...) {
    if (rank == 0) {
      printf("EXCEPTION in empty subset test\n");
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Test 2: MPI_INT8_T
  for (const auto& subset : subsets) {
    test_count++;
    int tag = test_count;
    if (test_builtin_type<int8_t>(subset, MPI_INT8_T, tag)) {
      pass_count++;
    } else if (rank == 0) {
      printf("FAILED: MPI_INT8_T with subset %s\n", subset.str().c_str());
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Test 3: MPI_INT32_T
  for (const auto& subset : subsets) {
    test_count++;
    int tag = test_count;
    if (test_builtin_type<int32_t>(subset, MPI_INT32_T, tag)) {
      pass_count++;
    } else if (rank == 0) {
      printf("FAILED: MPI_INT32_T with subset %s\n", subset.str().c_str());
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Test 4: MPI_INT64_T
  for (const auto& subset : subsets) {
    test_count++;
    int tag = test_count;
    if (test_builtin_type<int64_t>(subset, MPI_INT64_T, tag)) {
      pass_count++;
    } else if (rank == 0) {
      printf("FAILED: MPI_INT64_T with subset %s\n", subset.str().c_str());
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Test 5: MPI_DOUBLE
  for (const auto& subset : subsets) {
    test_count++;
    int tag = test_count;
    if (test_builtin_type<double>(subset, MPI_DOUBLE, tag)) {
      pass_count++;
    } else if (rank == 0) {
      printf("FAILED: MPI_DOUBLE with subset %s\n", subset.str().c_str());
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Test 6: MPI_FLOAT
  for (const auto& subset : subsets) {
    test_count++;
    int tag = test_count;
    if (test_builtin_type<float>(subset, MPI_FLOAT, tag)) {
      pass_count++;
    } else if (rank == 0) {
      printf("FAILED: MPI_FLOAT with subset %s\n", subset.str().c_str());
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Test 7: Custom struct datatype
  for (const auto& subset : subsets) {
    test_count++;
    int tag = test_count;
    if (test_custom_struct_type(subset, tag)) {
      pass_count++;
    } else if (rank == 0) {
      printf("FAILED: Custom struct with subset %s\n", subset.str().c_str());
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Test 8: MPI vector datatype
  for (const auto& subset : subsets) {
    test_count++;
    int tag = test_count;
    if (test_vector_datatype(subset, tag)) {
      pass_count++;
    } else if (rank == 0) {
      printf("FAILED: Vector datatype with subset %s\n", subset.str().c_str());
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);

  if (rank == 0) {
    printf("\n=================================\n");
    printf("SUBSET TO_DATATYPE TESTS: %d / %d PASSED\n", pass_count, test_count);
    printf("=================================\n");

    if (pass_count == test_count) {
      printf("ALL TESTS PASSED\n");
    } else {
      printf("SOME TESTS FAILED\n");
    }
  }

  MPI_Finalize();
  return (pass_count == test_count) ? 0 : 1;
}
