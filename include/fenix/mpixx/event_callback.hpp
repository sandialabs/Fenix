#ifndef FENIX_MPIXX_EVENT_CALLBACK_HPP
#define FENIX_MPIXX_EVENT_CALLBACK_HPP

#include <mpi.h>

#include <functional>
#include <memory>
#include <string_view>

#include "fenix/mpixx/event_handle.hpp"
#include "fenix/mpixx/tools.hpp"

namespace fenix::mpixx {

class EventCallback {
 public:
  using Callback = std::function<void(MPI_T_event_instance, MPI_T_cb_safety)>;

  EventCallback() noexcept = default;
  ~EventCallback();

  EventCallback(EventCallback&& o) noexcept;
  EventCallback& operator=(EventCallback&& o) noexcept;

  EventCallback(const EventCallback&)            = delete;
  EventCallback& operator=(const EventCallback&) = delete;

  void register_event(
    std::shared_ptr<Tools> tools, std::string_view event_name,
    Callback callback,
    MPI_T_cb_safety cb_safety = MPI_T_CB_REQUIRE_MPI_RESTRICTED
  );

  void free();
  const EventHandle& handle() const noexcept { return handle_; }

  explicit operator bool() const noexcept { return static_cast<bool>(handle_); }

 private:
  EventHandle handle_;
  std::unique_ptr<Callback> callback_;
};

} // namespace fenix::mpixx

#endif // FENIX_MPIXX_EVENT_CALLBACK_HPP
