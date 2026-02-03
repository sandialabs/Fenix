#ifndef FENIX_TASKS_AWAITER_H
#define FENIX_TASKS_AWAITER_H

#include <coroutine>
#include <exception>

#include "subtask.h"

namespace fenix::tasks {

enum class AwaitMode { NonBlocking, Blocking };

template <Subtaskable T>
class Awaiter {
 public:
  using TaskT = Subtask<T>;

  Awaiter(Subtask<T> t) : task(t) {}
  Awaiter(SubtaskBase* t) : Awaiter(*static_cast<Subtask<T>*>(t)) {}

  bool await_ready() const { return task.done(); }
  auto await_resume() const noexcept { return task.result(); }

  template <typename PromiseT>
  bool await_suspend(std::coroutine_handle<PromiseT> h) const {
    if (h.promise().await_mode == AwaitMode::Blocking) task.wait();
    return !await_ready();
  }

  mutable Subtask<T> task;
};

} // namespace fenix::tasks

#endif //FENIX_TASKS_AWAITER_H
