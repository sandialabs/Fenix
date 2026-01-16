#ifndef FENIX_TASKS_PROMISE_H
#define FENIX_TASKS_PROMISE_H

#include <coroutine>
#include <exception>
#include "subtask.h"
#include "awaiter.h"
#include "request.h"

#include <cstdio>

namespace fenix::tasks {

namespace impl {
template <typename T>
struct ReturnHolder {
  T val;
  void return_value(T&& v) { val = std::move(v); }
  T result() { return val; }
};
template <>
struct ReturnHolder<void> {
  void return_void() {};
};
}

template <typename T, bool eager>
class Task;

template <typename T, bool eager = true>
class Promise : public impl::ReturnHolder<T> {
 public:
  using PromiseT = Promise<T, eager>;
  using TaskT = Task<T, eager>;
  using HandleT = std::coroutine_handle<PromiseT>;

  TaskT get_return_object() noexcept {
    assert(!handle);
    handle = HandleT::from_promise(*this);
    return {this};
  }

  // Eagerly start tasks
  auto initial_suspend() noexcept {
    if constexpr (eager) return std::suspend_never{};
    if constexpr (!eager) return std::suspend_always{};
  }
  // Don't destroy coroutine until object is destroyed
  auto final_suspend() noexcept {
    subtask.reset();
    coro_done = true;
    return std::suspend_always{};
  }
  // Rethrow exceptions immediately
  void unhandled_exception() { throw; }

  void destroy() { handle.destroy(); }
  bool done() { return coro_done; }

  void resume() {
    if (done()) return;
    if (subtask) {
      if (await_mode == AwaitMode::Blocking) subtask->wait();
      else subtask->resume();
      if (subtask->done()) subtask.reset();
    }
    if (!subtask) handle.resume();
  }
  void wait() {
    await_mode = AwaitMode::Blocking;
    while (!done()) resume();
  }

  template <Subtaskable U>
  Awaiter<U> await_transform(U&& u) {
    if constexpr (std::is_base_of_v<SubtaskBase, U>) {
      subtask = std::make_shared<U>(std::forward<U>(u));
    } else {
      subtask = std::make_shared<Subtask<U>>(std::forward<U>(u));
    }
    return subtask.get();
  }
  Awaiter<Request> await_transform(MPI_Request*& r) {
    return await_transform(Request(r));
  }
  Awaiter<Request> await_transform(MPI_Request& r) {
    return await_transform(Request(&r));
  }
  auto await_transform(const std::suspend_always& s) {
    subtask.reset();
    return std::suspend_always{};
  }

  HandleT handle;
  bool coro_done = false;
  AwaitMode await_mode = AwaitMode::NonBlocking;
  std::shared_ptr<SubtaskBase> subtask;
};

} // namespace fenix::tasks

#endif //FENIX_TASKS_PROMISE_H
