#include "fenix/mpixx/event_callback.hpp"

#include <mpi.h>

#include <utility>

namespace fenix::mpixx {
namespace {

void event_callback_trampoline(
  MPI_T_event_instance event, MPI_T_event_registration,
  MPI_T_cb_safety cb_safety, void* user_data
) {
  auto* callback = static_cast<EventCallback::Callback*>(user_data);
  if (callback == nullptr || !*callback) return;
  (*callback)(event, cb_safety);
}

void event_callback_free_trampoline(
  MPI_T_event_registration, MPI_T_cb_safety, void*
) {}

} // namespace

EventCallback::~EventCallback() { free(); }

EventCallback::EventCallback(EventCallback&& o) noexcept {
  *this = std::move(o);
}

EventCallback& EventCallback::operator=(EventCallback&& o) noexcept {
  if (this != &o) {
    free();
    handle_   = std::move(o.handle_);
    callback_ = std::move(o.callback_);
  }
  return *this;
}

void EventCallback::register_event(
  std::shared_ptr<Tools> tools, std::string_view event_name, Callback callback,
  MPI_T_cb_safety cb_safety
) {
  free();
  handle_.alloc(std::move(tools), event_name);
  if (!handle_) return;

  callback_ = std::make_unique<Callback>(std::move(callback));

  int ret = MPI_T_event_register_callback(
    handle_.get(), cb_safety, MPI_INFO_NULL, callback_.get(),
    event_callback_trampoline
  );
  if (ret != MPI_SUCCESS) free();
}

void EventCallback::free() {
  handle_.free(callback_.get(), event_callback_free_trampoline);
  callback_.reset();
}

} // namespace fenix::mpixx
