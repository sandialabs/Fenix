#ifndef FENIX_MPIXX_EVENT_HANDLE_HPP
#define FENIX_MPIXX_EVENT_HANDLE_HPP

#include <mpi.h>

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "fenix/mpixx/tools.hpp"

namespace fenix::mpixx {

class EventHandle {
 public:
  explicit EventHandle(
    MPI_T_event_registration registration = MPI_T_EVENT_REGISTRATION_NULL
  ) noexcept
    : registration_(registration) {}
  EventHandle(std::shared_ptr<Tools> tools, std::string_view event_name);
  ~EventHandle() { free(); }

  void alloc(std::shared_ptr<Tools> tools, std::string_view event_name);

  EventHandle(EventHandle&& o) noexcept { *this = std::move(o); }
  EventHandle& operator=(EventHandle&& o) noexcept;
  EventHandle& operator=(MPI_T_event_registration registration) noexcept {
    return *this = EventHandle(registration);
  }

  EventHandle(const EventHandle&)            = delete;
  EventHandle& operator=(const EventHandle&) = delete;

  MPI_T_event_registration get() const noexcept { return registration_; }
  explicit operator bool() const noexcept {
    return registration_ != MPI_T_EVENT_REGISTRATION_NULL;
  }

  std::vector<MPI_Datatype> datatypes() const;

  void free(
    void* user_data = nullptr, MPI_T_event_free_cb_function free_cb = nullptr
  ) noexcept;

 private:
  MPI_T_event_registration registration_ = MPI_T_EVENT_REGISTRATION_NULL;
  std::shared_ptr<Tools> tools_;
  int event_index_ = -1;
};

} // namespace fenix::mpixx

#endif // FENIX_MPIXX_EVENT_HANDLE_HPP
