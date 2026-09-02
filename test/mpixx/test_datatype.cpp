#include <fenix/mpixx/datatype.hpp>
#include <fenix_opt.hpp>
#include <mpi.h>
#include <cstdio>
#include <cstring>
#include <vector>

using fenix::mpixx::Datatype;

// Helper function to verify type properties match
void verify_type_properties(
  MPI_Datatype original, MPI_Datatype reconstructed, const char* test_name
) {
  int orig_size, recon_size;
  MPI_Type_size(original, &orig_size);
  MPI_Type_size(reconstructed, &recon_size);
  fenix_require(
    orig_size == recon_size,
    "%s: size mismatch (orig=%d, recon=%d)",
    test_name,
    orig_size,
    recon_size
  );

  MPI_Aint orig_lb, orig_extent, recon_lb, recon_extent;
  MPI_Type_get_extent(original, &orig_lb, &orig_extent);
  MPI_Type_get_extent(reconstructed, &recon_lb, &recon_extent);
  fenix_require(
    orig_lb == recon_lb,
    "%s: lower bound mismatch (orig=%ld, recon=%ld)",
    test_name,
    (long)orig_lb,
    (long)recon_lb
  );
  fenix_require(
    orig_extent == recon_extent,
    "%s: extent mismatch (orig=%ld, recon=%ld)",
    test_name,
    (long)orig_extent,
    (long)recon_extent
  );
}

// Test builtin types (should not be freed, should serialize/deserialize)
void test_builtin_types(int rank) {
  if (rank == 0) {
    printf("TEST: Builtin types\n");
  }

  // Test various builtin types
  MPI_Datatype builtins[] = {
    MPI_INT, MPI_DOUBLE, MPI_FLOAT, MPI_CHAR, MPI_LONG, MPI_UNSIGNED,
    MPI_BYTE, MPI_PACKED
  };

  for (auto builtin : builtins) {
    Datatype dt(builtin);
    fenix_require(dt.is_builtin(), "Type should be recognized as builtin");

    // Serialize and deserialize
    auto buffer = dt.serialize();
    fenix_require(!buffer.empty(), "Serialized buffer should not be empty");

    Datatype reconstructed = Datatype::deserialize(buffer);
    fenix_require(
      reconstructed.get() == builtin,
      "Deserialized builtin should match original"
    );
    fenix_require(
      reconstructed.is_builtin(), "Deserialized type should be builtin"
    );

    // Release the original (shouldn't free builtin)
    MPI_Datatype released = dt.release();
    fenix_require(released == builtin, "Released builtin should match");
  }

  if (rank == 0) {
    printf("  PASSED: Builtin types\n");
  }
}

// Test MPI_Type_contiguous
void test_contiguous(int rank) {
  if (rank == 0) {
    printf("TEST: Contiguous type\n");
  }

  // Create contiguous type: 10 doubles
  Datatype contig = Datatype::contiguous(10, MPI_DOUBLE);
  contig.commit();

  fenix_require(!contig.is_builtin(), "Contiguous type should not be builtin");
  fenix_require(
    contig.size() == 10 * sizeof(double), "Contiguous type size should be correct"
  );

  // Serialize and deserialize
  auto buffer = contig.serialize();
  Datatype reconstructed = Datatype::deserialize(buffer);
  reconstructed.commit();

  verify_type_properties(contig, reconstructed, "Contiguous");

  // Test actual communication with the type
  if (rank == 0) {
    double send_data[10];
    for (int i = 0; i < 10; i++) {
      send_data[i] = i * 1.5;
    }
    MPI_Send(send_data, 1, reconstructed, 1, 0, MPI_COMM_WORLD);
  } else if (rank == 1) {
    double recv_data[10] = {0};
    MPI_Recv(recv_data, 1, reconstructed, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    for (int i = 0; i < 10; i++) {
      fenix_require(
        recv_data[i] == i * 1.5, "Contiguous: received data mismatch at index %d", i
      );
    }
  }

  if (rank == 0) {
    printf("  PASSED: Contiguous type\n");
  }
}

// Test MPI_Type_vector
void test_vector(int rank) {
  if (rank == 0) {
    printf("TEST: Vector type\n");
  }

  // Create vector: 3 blocks of 2 ints, stride of 4 ints
  Datatype vec = Datatype::vector(3, 2, 4, MPI_INT);
  vec.commit();

  fenix_require(!vec.is_builtin(), "Vector type should not be builtin");

  // Serialize and deserialize
  auto buffer = vec.serialize();
  Datatype reconstructed = Datatype::deserialize(buffer);
  reconstructed.commit();

  verify_type_properties(vec, reconstructed, "Vector");

  // Test actual communication
  if (rank == 0) {
    int send_data[12] = {1, 2, 9, 9, 3, 4, 9, 9, 5, 6, 9, 9};
    MPI_Send(send_data, 1, reconstructed, 1, 1, MPI_COMM_WORLD);
  } else if (rank == 1) {
    int recv_data[12] = {0};
    MPI_Recv(recv_data, 1, reconstructed, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    // Should receive: 1,2,x,x,3,4,x,x,5,6,x,x (only indices 0,1,4,5,8,9)
    fenix_require(recv_data[0] == 1 && recv_data[1] == 2, "Vector: block 0 mismatch");
    fenix_require(recv_data[4] == 3 && recv_data[5] == 4, "Vector: block 1 mismatch");
    fenix_require(recv_data[8] == 5 && recv_data[9] == 6, "Vector: block 2 mismatch");
  }

  if (rank == 0) {
    printf("  PASSED: Vector type\n");
  }
}

// Test MPI_Type_indexed
void test_indexed(int rank) {
  if (rank == 0) {
    printf("TEST: Indexed type\n");
  }

  // Create indexed: blocks at different offsets
  int blocklengths[3] = {2, 3, 1};
  int displacements[3] = {0, 3, 8};
  Datatype indexed = Datatype::indexed(3, blocklengths, displacements, MPI_INT);
  indexed.commit();

  // Serialize and deserialize
  auto buffer = indexed.serialize();
  Datatype reconstructed = Datatype::deserialize(buffer);
  reconstructed.commit();

  verify_type_properties(indexed, reconstructed, "Indexed");

  // Test communication
  if (rank == 0) {
    int send_data[10] = {10, 20, 99, 30, 40, 50, 99, 99, 60, 99};
    MPI_Send(send_data, 1, reconstructed, 1, 2, MPI_COMM_WORLD);
  } else if (rank == 1) {
    int recv_data[10] = {0};
    MPI_Recv(recv_data, 1, reconstructed, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    fenix_require(
      recv_data[0] == 10 && recv_data[1] == 20, "Indexed: block 0 mismatch"
    );
    fenix_require(
      recv_data[3] == 30 && recv_data[4] == 40 && recv_data[5] == 50,
      "Indexed: block 1 mismatch"
    );
    fenix_require(recv_data[8] == 60, "Indexed: block 2 mismatch");
  }

  if (rank == 0) {
    printf("  PASSED: Indexed type\n");
  }
}

// Test MPI_Type_create_struct
void test_struct(int rank) {
  if (rank == 0) {
    printf("TEST: Struct type\n");
  }

  // Create struct with mixed types: int, double, char[3]
  struct TestStruct {
    int a;
    double b;
    char c[3];
  };

  int blocklengths[3] = {1, 1, 3};
  MPI_Aint displacements[3];
  displacements[0] = offsetof(TestStruct, a);
  displacements[1] = offsetof(TestStruct, b);
  displacements[2] = offsetof(TestStruct, c);
  MPI_Datatype types[3] = {MPI_INT, MPI_DOUBLE, MPI_CHAR};

  Datatype struct_type = Datatype::create_struct(3, blocklengths, displacements, types);
  struct_type.commit();

  // Serialize and deserialize
  auto buffer = struct_type.serialize();
  Datatype reconstructed = Datatype::deserialize(buffer);
  reconstructed.commit();

  verify_type_properties(struct_type, reconstructed, "Struct");

  // Test communication
  if (rank == 0) {
    TestStruct send_val = {42, 3.14159, {'X', 'Y', 'Z'}};
    MPI_Send(&send_val, 1, reconstructed, 1, 3, MPI_COMM_WORLD);
  } else if (rank == 1) {
    TestStruct recv_val = {0, 0.0, {0, 0, 0}};
    MPI_Recv(&recv_val, 1, reconstructed, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    fenix_require(recv_val.a == 42, "Struct: int field mismatch");
    fenix_require(
      recv_val.b > 3.14 && recv_val.b < 3.15, "Struct: double field mismatch"
    );
    fenix_require(
      recv_val.c[0] == 'X' && recv_val.c[1] == 'Y' && recv_val.c[2] == 'Z',
      "Struct: char array field mismatch"
    );
  }

  if (rank == 0) {
    printf("  PASSED: Struct type\n");
  }
}

// Test nested custom types
void test_nested_types(int rank) {
  if (rank == 0) {
    printf("TEST: Nested custom types\n");
  }

  // Create a base type: contiguous 2 ints
  Datatype base = Datatype::contiguous(2, MPI_INT);
  base.commit();

  // Create nested type: vector of the base type
  Datatype nested = Datatype::vector(3, 1, 2, base);
  nested.commit();

  // Serialize and deserialize
  auto buffer = nested.serialize();
  Datatype reconstructed = Datatype::deserialize(buffer);
  reconstructed.commit();

  verify_type_properties(nested, reconstructed, "Nested");

  // Test communication
  if (rank == 0) {
    // Layout: [pair0] gap [pair1] gap [pair2] gap
    int send_data[12] = {1, 2, 99, 99, 3, 4, 99, 99, 5, 6, 99, 99};
    MPI_Send(send_data, 1, reconstructed, 1, 4, MPI_COMM_WORLD);
  } else if (rank == 1) {
    int recv_data[12] = {0};
    MPI_Recv(recv_data, 1, reconstructed, 0, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    fenix_require(
      recv_data[0] == 1 && recv_data[1] == 2, "Nested: pair 0 mismatch"
    );
    fenix_require(
      recv_data[4] == 3 && recv_data[5] == 4, "Nested: pair 1 mismatch"
    );
    fenix_require(
      recv_data[8] == 5 && recv_data[9] == 6, "Nested: pair 2 mismatch"
    );
  }

  if (rank == 0) {
    printf("  PASSED: Nested custom types\n");
  }
}

// Test MPI_Type_create_resized
void test_resized(int rank) {
  if (rank == 0) {
    printf("TEST: Resized type\n");
  }

  // Create base type: 3 ints
  Datatype base = Datatype::contiguous(3, MPI_INT);
  base.commit();

  // Get original extent
  MPI_Aint orig_lb, orig_extent;
  base.get_extent(&orig_lb, &orig_extent);

  // Resize to have extent of 5 ints (adds padding)
  MPI_Aint new_extent = 5 * sizeof(int);
  Datatype resized = Datatype::resized(base, 0, new_extent);
  resized.commit();

  // Verify new extent
  MPI_Aint new_lb, actual_extent;
  resized.get_extent(&new_lb, &actual_extent);
  fenix_require(
    actual_extent == new_extent,
    "Resized: extent should be %ld, got %ld",
    (long)new_extent,
    (long)actual_extent
  );

  // Serialize and deserialize
  auto buffer = resized.serialize();
  Datatype reconstructed = Datatype::deserialize(buffer);
  reconstructed.commit();

  verify_type_properties(resized, reconstructed, "Resized");

  // Test communication with array of resized types
  if (rank == 0) {
    // Send 2 instances: each has 3 valid ints + 2 padding ints
    int send_data[10] = {1, 2, 3, 99, 99, 4, 5, 6, 99, 99};
    MPI_Send(send_data, 2, reconstructed, 1, 5, MPI_COMM_WORLD);
  } else if (rank == 1) {
    int recv_data[10] = {0};
    MPI_Recv(recv_data, 2, reconstructed, 0, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    fenix_require(
      recv_data[0] == 1 && recv_data[1] == 2 && recv_data[2] == 3,
      "Resized: first instance mismatch"
    );
    fenix_require(
      recv_data[5] == 4 && recv_data[6] == 5 && recv_data[7] == 6,
      "Resized: second instance mismatch"
    );
  }

  if (rank == 0) {
    printf("  PASSED: Resized type\n");
  }
}

// Test cross-rank serialization: rank 0 creates, serializes, sends to rank 1
// Rank 1 deserializes and verifies
void test_cross_rank_serialization(int rank) {
  if (rank == 0) {
    printf("TEST: Cross-rank serialization\n");
  }

  std::vector<uint8_t> buffer;
  Datatype dt;

  if (rank == 0) {
    // Rank 0: Create a complex type (indexed block)
    int blocklength = 2;
    int displacements[4] = {0, 3, 7, 10};
    dt = Datatype::indexed_block(4, blocklength, displacements, MPI_DOUBLE);
    dt.commit();

    // Serialize
    buffer = dt.serialize();

    // Send buffer size then buffer
    int buffer_size = buffer.size();
    MPI_Send(&buffer_size, 1, MPI_INT, 1, 6, MPI_COMM_WORLD);
    MPI_Send(buffer.data(), buffer_size, MPI_BYTE, 1, 7, MPI_COMM_WORLD);

    // Send test data
    double send_data[15] = {
      1.0, 2.0,  99,    3.0, 4.0,  99,    99,   5.0,
      6.0, 99,   7.0,   8.0, 99,   99,    99
    };
    MPI_Send(send_data, 1, dt, 1, 8, MPI_COMM_WORLD);
  } else if (rank == 1) {
    // Rank 1: Receive and deserialize
    int buffer_size;
    MPI_Recv(&buffer_size, 1, MPI_INT, 0, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    buffer.resize(buffer_size);
    MPI_Recv(buffer.data(), buffer_size, MPI_BYTE, 0, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Deserialize
    dt = Datatype::deserialize(buffer);
    dt.commit();

    // Verify properties
    int size;
    MPI_Type_size(dt, &size);
    fenix_require(
      size == 8 * sizeof(double), "Cross-rank: size should be 8 doubles"
    );

    // Receive test data
    double recv_data[15] = {0};
    MPI_Recv(recv_data, 1, dt, 0, 8, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Verify received data
    fenix_require(
      recv_data[0] == 1.0 && recv_data[1] == 2.0,
      "Cross-rank: block 0 mismatch"
    );
    fenix_require(
      recv_data[3] == 3.0 && recv_data[4] == 4.0,
      "Cross-rank: block 1 mismatch"
    );
    fenix_require(
      recv_data[7] == 5.0 && recv_data[8] == 6.0,
      "Cross-rank: block 2 mismatch"
    );
    fenix_require(
      recv_data[10] == 7.0 && recv_data[11] == 8.0,
      "Cross-rank: block 3 mismatch"
    );
  }

  if (rank == 0) {
    printf("  PASSED: Cross-rank serialization\n");
  }
}

// Test hvector (vector with byte stride)
void test_hvector(int rank) {
  if (rank == 0) {
    printf("TEST: Hvector type\n");
  }

  // Create hvector: 3 blocks of 2 ints, byte stride
  MPI_Aint stride = 5 * sizeof(int); // Larger than needed
  Datatype hvec = Datatype::hvector(3, 2, stride, MPI_INT);
  hvec.commit();

  // Serialize and deserialize
  auto buffer = hvec.serialize();
  Datatype reconstructed = Datatype::deserialize(buffer);
  reconstructed.commit();

  verify_type_properties(hvec, reconstructed, "Hvector");

  // Test communication
  if (rank == 0) {
    int send_data[15] = {1, 2, 9, 9, 9, 3, 4, 9, 9, 9, 5, 6, 9, 9, 9};
    MPI_Send(send_data, 1, reconstructed, 1, 9, MPI_COMM_WORLD);
  } else if (rank == 1) {
    int recv_data[15] = {0};
    MPI_Recv(recv_data, 1, reconstructed, 0, 9, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    fenix_require(
      recv_data[0] == 1 && recv_data[1] == 2, "Hvector: block 0 mismatch"
    );
    fenix_require(
      recv_data[5] == 3 && recv_data[6] == 4, "Hvector: block 1 mismatch"
    );
    fenix_require(
      recv_data[10] == 5 && recv_data[11] == 6, "Hvector: block 2 mismatch"
    );
  }

  if (rank == 0) {
    printf("  PASSED: Hvector type\n");
  }
}

// Test hindexed (indexed with byte displacements)
void test_hindexed(int rank) {
  if (rank == 0) {
    printf("TEST: Hindexed type\n");
  }

  int blocklengths[2] = {3, 2};
  MPI_Aint displacements[2] = {0, 6 * sizeof(int)};
  Datatype hindexed = Datatype::hindexed(2, blocklengths, displacements, MPI_INT);
  hindexed.commit();

  // Serialize and deserialize
  auto buffer = hindexed.serialize();
  Datatype reconstructed = Datatype::deserialize(buffer);
  reconstructed.commit();

  verify_type_properties(hindexed, reconstructed, "Hindexed");

  // Test communication
  if (rank == 0) {
    int send_data[10] = {10, 20, 30, 99, 99, 99, 40, 50, 99, 99};
    MPI_Send(send_data, 1, reconstructed, 1, 10, MPI_COMM_WORLD);
  } else if (rank == 1) {
    int recv_data[10] = {0};
    MPI_Recv(recv_data, 1, reconstructed, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    fenix_require(
      recv_data[0] == 10 && recv_data[1] == 20 && recv_data[2] == 30,
      "Hindexed: block 0 mismatch"
    );
    fenix_require(
      recv_data[6] == 40 && recv_data[7] == 50, "Hindexed: block 1 mismatch"
    );
  }

  if (rank == 0) {
    printf("  PASSED: Hindexed type\n");
  }
}

// Test indexed_block
void test_indexed_block(int rank) {
  if (rank == 0) {
    printf("TEST: Indexed_block type\n");
  }

  // All blocks have same length, but different displacements
  int blocklength = 2;
  int displacements[3] = {0, 4, 9};
  Datatype iblock = Datatype::indexed_block(3, blocklength, displacements, MPI_INT);
  iblock.commit();

  // Serialize and deserialize
  auto buffer = iblock.serialize();
  Datatype reconstructed = Datatype::deserialize(buffer);
  reconstructed.commit();

  verify_type_properties(iblock, reconstructed, "Indexed_block");

  // Test communication
  if (rank == 0) {
    int send_data[12] = {10, 20, 99, 99, 30, 40, 99, 99, 99, 50, 60, 99};
    MPI_Send(send_data, 1, reconstructed, 1, 11, MPI_COMM_WORLD);
  } else if (rank == 1) {
    int recv_data[12] = {0};
    MPI_Recv(recv_data, 1, reconstructed, 0, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    fenix_require(
      recv_data[0] == 10 && recv_data[1] == 20, "Indexed_block: block 0 mismatch"
    );
    fenix_require(
      recv_data[4] == 30 && recv_data[5] == 40, "Indexed_block: block 1 mismatch"
    );
    fenix_require(
      recv_data[9] == 50 && recv_data[10] == 60, "Indexed_block: block 2 mismatch"
    );
  }

  if (rank == 0) {
    printf("  PASSED: Indexed_block type\n");
  }
}

// Test hindexed_block
void test_hindexed_block(int rank) {
  if (rank == 0) {
    printf("TEST: Hindexed_block type\n");
  }

  int blocklength = 3;
  MPI_Aint displacements[2] = {0, 7 * sizeof(int)};
  Datatype hblock = Datatype::hindexed_block(2, blocklength, displacements, MPI_INT);
  hblock.commit();

  // Serialize and deserialize
  auto buffer = hblock.serialize();
  Datatype reconstructed = Datatype::deserialize(buffer);
  reconstructed.commit();

  verify_type_properties(hblock, reconstructed, "Hindexed_block");

  // Test communication
  if (rank == 0) {
    int send_data[12] = {1, 2, 3, 99, 99, 99, 99, 4, 5, 6, 99, 99};
    MPI_Send(send_data, 1, reconstructed, 1, 12, MPI_COMM_WORLD);
  } else if (rank == 1) {
    int recv_data[12] = {0};
    MPI_Recv(recv_data, 1, reconstructed, 0, 12, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    fenix_require(
      recv_data[0] == 1 && recv_data[1] == 2 && recv_data[2] == 3,
      "Hindexed_block: block 0 mismatch"
    );
    fenix_require(
      recv_data[7] == 4 && recv_data[8] == 5 && recv_data[9] == 6,
      "Hindexed_block: block 1 mismatch"
    );
  }

  if (rank == 0) {
    printf("  PASSED: Hindexed_block type\n");
  }
}

// Test subarray
void test_subarray(int rank) {
  if (rank == 0) {
    printf("TEST: Subarray type\n");
  }

  // 5x5 array, extract 3x3 subarray starting at (1,1)
  int ndims = 2;
  int sizes[2] = {5, 5};
  int subsizes[2] = {3, 3};
  int starts[2] = {1, 1};
  Datatype subarray =
    Datatype::subarray(ndims, sizes, subsizes, starts, MPI_ORDER_C, MPI_INT);
  subarray.commit();

  // Serialize and deserialize
  auto buffer = subarray.serialize();
  Datatype reconstructed = Datatype::deserialize(buffer);
  reconstructed.commit();

  verify_type_properties(subarray, reconstructed, "Subarray");

  // Test communication
  if (rank == 0) {
    int send_data[25];
    for (int i = 0; i < 25; i++) send_data[i] = i;
    // Subarray extracts: rows 1-3, cols 1-3
    MPI_Send(send_data, 1, reconstructed, 1, 13, MPI_COMM_WORLD);
  } else if (rank == 1) {
    int recv_data[25] = {0};
    MPI_Recv(recv_data, 1, reconstructed, 0, 13, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    // Should receive elements at positions: 6,7,8, 11,12,13, 16,17,18
    fenix_require(recv_data[6] == 6, "Subarray: element (1,1) mismatch");
    fenix_require(recv_data[7] == 7, "Subarray: element (1,2) mismatch");
    fenix_require(recv_data[8] == 8, "Subarray: element (1,3) mismatch");
    fenix_require(recv_data[12] == 12, "Subarray: element (2,2) mismatch");
    fenix_require(recv_data[18] == 18, "Subarray: element (3,3) mismatch");
  }

  if (rank == 0) {
    printf("  PASSED: Subarray type\n");
  }
}

// Test darray
void test_darray(int rank, int size) {
  if (rank == 0) {
    printf("TEST: Darray type\n");
  }

  // Simple 1D block distribution across ranks
  int ndims = 1;
  int gsizes[1] = {8};  // Global size
  int distribs[1] = {MPI_DISTRIBUTE_BLOCK};
  int dargs[1] = {MPI_DISTRIBUTE_DFLT_DARG};
  int psizes[1] = {size};

  Datatype darray = Datatype::darray(
    size, rank, ndims, gsizes, distribs, dargs, psizes, MPI_ORDER_C, MPI_INT
  );
  darray.commit();

  // Serialize and deserialize
  auto buffer = darray.serialize();
  Datatype reconstructed = Datatype::deserialize(buffer);
  reconstructed.commit();

  verify_type_properties(darray, reconstructed, "Darray");

  if (rank == 0) {
    printf("  PASSED: Darray type\n");
  }
}

// Test DatatypeRef (non-owning reference)
void test_datatype_ref(int rank) {
  if (rank == 0) {
    printf("TEST: DatatypeRef (non-owning reference)\n");
  }

  using fenix::mpixx::DatatypeRef;

  // Create a custom type that we'll manage manually
  MPI_Datatype raw_type;
  MPI_Type_contiguous(5, MPI_INT, &raw_type);
  MPI_Type_commit(&raw_type);

  {
    // DatatypeRef should not free on destruction
    DatatypeRef ref(raw_type);
    fenix_require(ref.get() == raw_type, "DatatypeRef should hold the type");
    fenix_require(ref, "DatatypeRef should be truthy");
    fenix_require(!ref.is_builtin(), "Custom type should not be builtin");

    // Test size/extent work through ref
    fenix_require(
      ref.size() == 5 * sizeof(int), "DatatypeRef size should work"
    );

    // Test implicit conversion
    MPI_Datatype converted = ref;
    fenix_require(converted == raw_type, "Implicit conversion should work");
  }

  // After DatatypeRef destroyed, type should still be valid
  int test_size;
  int ret = MPI_Type_size(raw_type, &test_size);
  fenix_require(
    ret == MPI_SUCCESS,
    "Type should still be valid after DatatypeRef destruction"
  );
  fenix_require(
    test_size == 5 * sizeof(int), "Type should still work correctly"
  );

  // Test copying
  {
    DatatypeRef ref1(raw_type);
    DatatypeRef ref2(ref1);  // Copy constructor
    fenix_require(ref1.get() == raw_type, "Source should be unchanged");
    fenix_require(ref2.get() == raw_type, "Copy should reference same type");

    DatatypeRef ref3(MPI_INT);
    ref3 = ref1;  // Copy assignment
    fenix_require(ref3.get() == raw_type, "Copy assignment should work");
  }

  // Clean up manually
  MPI_Type_free(&raw_type);

  if (rank == 0) {
    printf("  PASSED: DatatypeRef\n");
  }
}

// Test move semantics
void test_move_semantics(int rank) {
  if (rank == 0) {
    printf("TEST: Move semantics\n");
  }

  // Test move constructor
  {
    Datatype dt1 = Datatype::contiguous(7, MPI_DOUBLE);
    dt1.commit();
    MPI_Datatype raw = dt1.get();
    fenix_require(raw != MPI_DATATYPE_NULL, "dt1 should have valid handle");

    Datatype dt2 = std::move(dt1);
    fenix_require(!dt1, "After move, source should be empty");
    fenix_require(
      dt1.get() == MPI_DATATYPE_NULL, "After move, source should be NULL"
    );
    fenix_require(dt2, "After move, destination should be valid");
    fenix_require(dt2.get() == raw, "Move should preserve handle");
  }

  // Test move assignment
  {
    Datatype dt1 = Datatype::contiguous(3, MPI_INT);
    dt1.commit();
    Datatype dt2 = Datatype::contiguous(5, MPI_INT);
    dt2.commit();

    MPI_Datatype raw1 = dt1.get();
    MPI_Datatype raw2 = dt2.get();

    fenix_require(raw1 != MPI_DATATYPE_NULL, "dt1 should be valid");
    fenix_require(raw2 != MPI_DATATYPE_NULL, "dt2 should be valid");
    fenix_require(raw1 != raw2, "dt1 and dt2 should be different");

    dt1 = std::move(dt2);
    fenix_require(!dt2, "After move assignment, source should be empty");
    fenix_require(dt1, "After move assignment, destination should be valid");
    fenix_require(dt1.get() == raw2, "Move assignment should transfer handle");
  }

  if (rank == 0) {
    printf("  PASSED: Move semantics\n");
  }
}

// Test release() on custom types
void test_release_custom(int rank) {
  if (rank == 0) {
    printf("TEST: release() on custom types\n");
  }

  Datatype dt = Datatype::contiguous(4, MPI_FLOAT);
  dt.commit();

  MPI_Datatype raw = dt.get();
  fenix_require(raw != MPI_DATATYPE_NULL, "Type should be valid");

  // Release ownership
  MPI_Datatype released = dt.release();
  fenix_require(released == raw, "Released handle should match");
  fenix_require(!dt, "After release, Datatype should be empty");
  fenix_require(dt.get() == MPI_DATATYPE_NULL, "After release, get() should return NULL");

  // Verify type is still valid (not freed)
  int size;
  int ret = MPI_Type_size(released, &size);
  fenix_require(ret == MPI_SUCCESS, "Released type should still be valid");
  fenix_require(
    size == 4 * sizeof(float), "Released type should work correctly"
  );

  // Clean up manually
  MPI_Type_free(&released);

  if (rank == 0) {
    printf("  PASSED: release() on custom types\n");
  }
}

// Test operator bool and get_true_extent
void test_misc_features(int rank) {
  if (rank == 0) {
    printf("TEST: operator bool() and get_true_extent()\n");
  }

  // Test operator bool
  {
    Datatype empty;
    fenix_require(!empty, "Empty Datatype should be falsy");

    Datatype valid(MPI_INT);
    fenix_require(valid, "Valid Datatype should be truthy");

    Datatype custom = Datatype::contiguous(2, MPI_INT);
    custom.commit();
    fenix_require(custom, "Custom type should be truthy");

    MPI_Datatype released = custom.release();
    fenix_require(!custom, "Released Datatype should be falsy");
    MPI_Type_free(&released);
  }

  // Test get_true_extent
  {
    Datatype dt = Datatype::contiguous(10, MPI_INT);
    dt.commit();

    MPI_Aint true_lb, true_extent;
    dt.get_true_extent(&true_lb, &true_extent);

    fenix_require(true_lb == 0, "True lower bound should be 0");
    fenix_require(
      true_extent == 10 * sizeof(int),
      "True extent should be 10 ints"
    );
  }

  if (rank == 0) {
    printf("  PASSED: operator bool() and get_true_extent()\n");
  }
}

// Test edge cases
void test_edge_cases(int rank) {
  if (rank == 0) {
    printf("TEST: Edge cases\n");
  }

  // Zero-count contiguous (should create valid but empty type)
  {
    Datatype zero_count = Datatype::contiguous(0, MPI_INT);
    zero_count.commit();
    fenix_require(zero_count.size() == 0, "Zero-count type should have size 0");

    auto buffer = zero_count.serialize();
    Datatype reconstructed = Datatype::deserialize(buffer);
    reconstructed.commit();
    fenix_require(
      reconstructed.size() == 0, "Reconstructed zero-count should have size 0"
    );
  }

  // Deeply nested types (3 levels)
  {
    Datatype level1 = Datatype::contiguous(2, MPI_INT);
    level1.commit();

    Datatype level2 = Datatype::contiguous(3, level1);
    level2.commit();

    Datatype level3 = Datatype::contiguous(4, level2);
    level3.commit();

    auto buffer = level3.serialize();
    Datatype reconstructed = Datatype::deserialize(buffer);
    reconstructed.commit();

    // Should represent 4*3*2 = 24 ints
    fenix_require(
      reconstructed.size() == 24 * sizeof(int),
      "Deeply nested type size mismatch"
    );
  }

  // Resized with non-zero lower bound
  {
    Datatype base = Datatype::contiguous(3, MPI_INT);
    base.commit();

    MPI_Aint new_lb = 8;  // Non-zero lower bound
    MPI_Aint new_extent = 20;
    Datatype resized = Datatype::resized(base, new_lb, new_extent);
    resized.commit();

    MPI_Aint lb, extent;
    resized.get_extent(&lb, &extent);
    fenix_require(lb == new_lb, "Resized: lower bound should be %ld, got %ld", (long)new_lb, (long)lb);
    fenix_require(extent == new_extent, "Resized: extent should be %ld, got %ld", (long)new_extent, (long)extent);

    // Serialize and verify
    auto buffer = resized.serialize();
    Datatype reconstructed = Datatype::deserialize(buffer);
    reconstructed.commit();

    MPI_Aint recon_lb, recon_extent;
    reconstructed.get_extent(&recon_lb, &recon_extent);
    fenix_require(
      recon_lb == new_lb, "Reconstructed: lower bound mismatch"
    );
    fenix_require(
      recon_extent == new_extent, "Reconstructed: extent mismatch"
    );
  }

  if (rank == 0) {
    printf("  PASSED: Edge cases\n");
  }
}

// Test error conditions
void test_error_conditions(int rank) {
  if (rank == 0) {
    printf("TEST: Error conditions\n");
  }

  // Test truncated buffer (too short)
  {
    bool caught_exception = false;
    try {
      std::vector<uint8_t> truncated(3);  // Too short (header is 4 bytes)
      Datatype dt = Datatype::deserialize(truncated);
    } catch (const std::exception& e) {
      caught_exception = true;
      if (rank == 0) {
        printf("    Caught expected exception for truncated buffer: %s\n", e.what());
      }
    }
    fenix_require(
      caught_exception, "Truncated buffer should throw exception"
    );
  }

  // Test invalid magic number
  {
    bool caught_exception = false;
    try {
      std::vector<uint8_t> bad_magic(20, 0);
      // Set wrong magic number
      uint32_t wrong_magic = 0xDEADBEEF;
      std::memcpy(bad_magic.data(), &wrong_magic, sizeof(wrong_magic));
      Datatype dt = Datatype::deserialize(bad_magic);
    } catch (const std::exception& e) {
      caught_exception = true;
      if (rank == 0) {
        printf("    Caught expected exception for invalid magic: %s\n", e.what());
      }
    }
    fenix_require(
      caught_exception, "Invalid magic number should throw exception"
    );
  }

  // Test buffer underrun during deserialization
  {
    bool caught_exception = false;
    try {
      // Create a valid type, serialize it, then truncate the buffer
      Datatype dt = Datatype::contiguous(10, MPI_INT);
      dt.commit();
      auto buffer = dt.serialize();

      // Truncate to just past header (will fail reading the type data)
      // Header is 4 bytes, so truncate to 6 bytes (won't have full int32 count + child)
      buffer.resize(6);
      Datatype reconstructed = Datatype::deserialize(buffer);
    } catch (const std::exception& e) {
      caught_exception = true;
      if (rank == 0) {
        printf("    Caught expected exception for buffer underrun: %s\n", e.what());
      }
    }
    fenix_require(
      caught_exception, "Buffer underrun should throw exception"
    );
  }

  if (rank == 0) {
    printf("  PASSED: Error conditions\n");
  }
}

// Test more builtin types
void test_more_builtins(int rank) {
  if (rank == 0) {
    printf("TEST: Additional builtin types\n");
  }

  MPI_Datatype more_builtins[] = {
    MPI_INT8_T, MPI_INT16_T, MPI_INT32_T, MPI_INT64_T,
    MPI_UINT8_T, MPI_UINT16_T, MPI_UINT32_T, MPI_UINT64_T,
    MPI_C_BOOL, MPI_AINT, MPI_COUNT, MPI_OFFSET,
    MPI_2INT, MPI_FLOAT_INT, MPI_DOUBLE_INT, MPI_LONG_INT
  };

  for (auto builtin : more_builtins) {
    Datatype dt(builtin);
    fenix_require(dt.is_builtin(), "Type should be recognized as builtin");

    auto buffer = dt.serialize();
    fenix_require(
      buffer.size() == 5, "Builtin serialization should be 5 bytes (header + combiner)"
    );

    Datatype reconstructed = Datatype::deserialize(buffer);
    fenix_require(
      reconstructed.get() == builtin, "Deserialized builtin should match"
    );
  }

  if (rank == 0) {
    printf("  PASSED: Additional builtin types\n");
  }
}

// Test MPI_DATATYPE_NULL serialization
void test_datatype_null(int rank) {
  if (rank == 0) {
    printf("TEST: MPI_DATATYPE_NULL serialization\n");
  }

  // Test serializing NULL
  {
    Datatype dt(MPI_DATATYPE_NULL);
    fenix_require(!dt, "NULL datatype should be falsy");
    fenix_require(dt.get() == MPI_DATATYPE_NULL, "Should hold NULL");

    auto buffer = dt.serialize();
    fenix_require(
      buffer.size() == 5, "NULL serialization should be 5 bytes (header + NULL marker)"
    );

    Datatype reconstructed = Datatype::deserialize(buffer);
    fenix_require(
      reconstructed.get() == MPI_DATATYPE_NULL,
      "Deserialized NULL should be MPI_DATATYPE_NULL"
    );
    fenix_require(!reconstructed, "Deserialized NULL should be falsy");
  }

  // Test NULL in a struct (nested)
  if (rank == 0) {
    int blocklengths[2] = {1, 1};
    MPI_Aint displacements[2] = {0, 8};
    MPI_Datatype types[2] = {MPI_INT, MPI_DATATYPE_NULL};

    // MPI_Type_create_struct should reject NULL types, so we test
    // that our serialization can handle it if it appears in introspection
    // For now, just test that NULL alone works correctly
  }

  if (rank == 0) {
    printf("  PASSED: MPI_DATATYPE_NULL\n");
  }
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  fenix_require(
    size >= 2,
    "This test requires at least 2 MPI ranks, got %d",
    size
  );

  // Run all tests
  test_builtin_types(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_contiguous(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_vector(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_indexed(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_struct(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_nested_types(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_resized(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_cross_rank_serialization(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_hvector(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_hindexed(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  // New tests for missing constructors
  test_indexed_block(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_hindexed_block(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_subarray(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_darray(rank, size);
  MPI_Barrier(MPI_COMM_WORLD);

  // Feature tests
  test_datatype_ref(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_move_semantics(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_release_custom(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_misc_features(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_edge_cases(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_error_conditions(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_more_builtins(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  test_datatype_null(rank);
  MPI_Barrier(MPI_COMM_WORLD);

  if (rank == 0) {
    printf("\n=================================\n");
    printf("ALL DATATYPE TESTS PASSED\n");
    printf("=================================\n");
  }

  MPI_Finalize();
  return 0;
}
