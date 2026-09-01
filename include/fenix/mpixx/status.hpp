#ifndef FENIX_MPIXX_STATUS_HPP
#define FENIX_MPIXX_STATUS_HPP

#include <mpi.h>
#include <tuple>

namespace fenix::mpixx {
class Status {
 public:
  int return_value = MPI_SUCCESS;
  MPI_Status status;

  Status() = default;
  Status(int r) : return_value(r) {}
  auto operator=(int r) {
    return_value = r;
    return *this;
  }

  operator bool() const { return return_value == MPI_SUCCESS; }

  operator int() const { return return_value; }
  bool operator==(int r) const { return return_value == r; }

  operator MPI_Status() const { return status; }
  operator MPI_Status*() { return &status; }

  // to support structured unbinding
  template <size_t I>
  auto&& get() && {
    if constexpr (I == 0) return std::move(return_value);
    if constexpr (I == 1) return std::move(status);
  }
};
} // namespace fenix::mpixx

// Supporting structured unbinding for Status
namespace std {
template <>
struct tuple_size<fenix::mpixx::Status> : std::integral_constant<size_t, 2> {};

template <>
struct tuple_element<0, fenix::mpixx::Status> {
  using type = int;
};
template <>
struct tuple_element<1, fenix::mpixx::Status> {
  using type = MPI_Status;
};
} // namespace std

#endif // FENIX_MPIXX_STATUS_HPP
