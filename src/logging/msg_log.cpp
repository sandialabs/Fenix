#include "fenix/logging/op_log.h"
#include "fenix/logging/serialize.h"

using namespace fenix::mpixx;

template <typename T>
struct MPIObjectRecords {
  void add(T obj) {
    if (!has(obj)) records.push_back({next_idx(), obj});
  }

  int find(const T& o) {
    for (auto& [idx, obj] : records)
      if (o == obj) return idx;
    fatal_print("Using unrecorded MPI_Type or MPI_Op %p\n", o);
  }
  T find(const int& i) {
    for (auto& [idx, obj] : records)
      if (i == idx) return obj;
    fatal_print("Recovering unrecorded MPI_Datatype or MPI_Op with id %d\n", i);
  }

  bool has(const T& o) {
    for (auto& [idx, obj] : records)
      if (o == obj) return true;
    return false;
  }
  bool has(const int& i) {
    for (auto& [idx, obj] : records)
      if (i == idx) return true;
    return false;
  }

 private:
  int next_idx() { return records.empty() ? 0 : records.back().first + 1; }
  std::vector<std::pair<int, T>> records;
};

MPIObjectRecords<MPI_Datatype> mpi_types;
MPIObjectRecords<MPI_Op> mpi_ops;

namespace fenix::logging {

void record_mpi_type(MPI_Datatype d) { mpi_types.add(d); }
void record_mpi_op(MPI_Op o) { mpi_ops.add(o); }

void init_mpi_records() {
  // clang-format off
  const std::vector<MPI_Datatype> builtin_mpi_types = {
    MPI_SIGNED_CHAR, MPI_UNSIGNED_CHAR, MPI_SHORT, MPI_UNSIGNED_SHORT, MPI_INT,
    MPI_UNSIGNED, MPI_LONG, MPI_UNSIGNED_LONG, MPI_LONG_LONG_INT, MPI_LONG_LONG,
    MPI_UNSIGNED_LONG_LONG, MPI_CHAR, MPI_WCHAR, MPI_FLOAT, MPI_DOUBLE,
    MPI_LONG_DOUBLE, MPI_INT8_T, MPI_UINT8_T, MPI_INT16_T, MPI_UINT16_T,
    MPI_INT32_T, MPI_UINT32_T, MPI_INT64_T, MPI_UINT64_T, MPI_C_BOOL,
    MPI_C_COMPLEX, MPI_C_FLOAT_COMPLEX, MPI_C_DOUBLE_COMPLEX,
    MPI_C_LONG_DOUBLE_COMPLEX, MPI_AINT, MPI_COUNT, MPI_OFFSET, MPI_BYTE,
    MPI_PACKED, MPI_SHORT_INT, MPI_LONG_INT, MPI_FLOAT_INT, MPI_DOUBLE_INT,
    MPI_2INT, MPI_DATATYPE_NULL
  };
  // clang-format on
  if (!mpi_types.has(builtin_mpi_types.back()))
    for (auto& datatype : builtin_mpi_types) record_mpi_type(datatype);

  const std::vector<MPI_Op> builtin_mpi_ops = {
    MPI_MAX, MPI_MIN, MPI_SUM,  MPI_PROD, MPI_LAND,   MPI_BAND,
    MPI_LOR, MPI_BOR, MPI_LXOR, MPI_BXOR, MPI_MAXLOC, MPI_MINLOC
  };
  if (!mpi_ops.has(builtin_mpi_ops.back()))
    for (auto& op : builtin_mpi_ops) record_mpi_op(op);
}
} //namespace fenix::logging

namespace fenix::logging::serialize {
void write(std::ostream& s, const MPI_Datatype& d) {
  write<int>(s, mpi_types.find(d));
}
void write(std::ostream& s, const MPI_Op& o) { write<int>(s, mpi_ops.find(o)); }
void read(std::istream& s, MPI_Datatype& d) {
  int id = read<int>(s);
  d      = mpi_types.find(id);
}
void read(std::istream& s, MPI_Op& o) {
  int id = read<int>(s);
  o      = mpi_ops.find(id);
}
} //namespace fenix::logging::serialize
