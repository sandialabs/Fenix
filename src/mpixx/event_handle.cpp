#include "fenix/mpixx/event_handle.hpp"

#include <mpi.h>

#include <utility>

#include "fenix_opt.hpp"

namespace fenix::mpixx {

EventHandle::EventHandle(
  std::shared_ptr<Tools> tools, std::string_view event_name
) {
  alloc(std::move(tools), event_name);
}

void EventHandle::alloc(
  std::shared_ptr<Tools> tools, std::string_view event_name
) {
  free();

  int ret = MPI_T_event_get_index(event_name.data(), &event_index_);
  if (ret != MPI_SUCCESS) {
    event_index_ = -1;
    return;
  }

  ret = MPI_T_event_handle_alloc(
    event_index_, nullptr, MPI_INFO_NULL, &registration_
  );
  if (ret != MPI_SUCCESS) {
    registration_ = MPI_T_EVENT_REGISTRATION_NULL;
    event_index_  = -1;
    return;
  }

  tools_ = std::move(tools);
}

std::vector<MPI_Datatype> EventHandle::datatypes() const {
  if (event_index_ < 0) return {};

  int num_elements = -1;
  int ret          = MPI_T_event_get_info(
    event_index_, NULL, NULL, NULL, NULL, NULL, &num_elements, NULL, NULL, NULL,
    NULL, NULL
  );
  fenix_assert(ret == MPI_SUCCESS);

  std::vector<MPI_Datatype> datatypes(num_elements);
  ret = MPI_T_event_get_info(
    event_index_, NULL, NULL, NULL, datatypes.data(), nullptr, &num_elements,
    NULL, NULL, NULL, NULL, NULL
  );
  fenix_assert(ret == MPI_SUCCESS);

  return datatypes;
}

EventHandle& EventHandle::operator=(EventHandle&& o) noexcept {
  if (this != &o) {
    free();
    registration_   = o.registration_;
    tools_          = std::move(o.tools_);
    event_index_    = o.event_index_;
    o.registration_ = MPI_T_EVENT_REGISTRATION_NULL;
    o.event_index_  = -1;
  }
  return *this;
}

void EventHandle::free(
  void* user_data, MPI_T_event_free_cb_function free_cb
) noexcept {
  if (registration_ != MPI_T_EVENT_REGISTRATION_NULL)
    MPI_T_event_handle_free(registration_, user_data, free_cb);
  registration_ = MPI_T_EVENT_REGISTRATION_NULL;
  tools_.reset();
  event_index_ = -1;
}

} // namespace fenix::mpixx
