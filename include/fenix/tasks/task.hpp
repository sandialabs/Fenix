#ifndef FENIX_TASKS_TASK_HPP
#define FENIX_TASKS_TASK_HPP

#include <cassert>
#include <memory>

#include "fenix/tasks/promise.hpp"
#include "fenix/tasks/forward.hpp"

namespace fenix::tasks {

// WARNING: eager tasks can cause gross double free errors, due to some
// early wonkiness in the C++ spec and in compiler implementations
template <typename T, bool eager /*= false*/>
class [[nodiscard("Must wait/await tasks")]] Task {
 public:
  using PromiseT     = Promise<T, eager>;
  using TaskT        = Task<T, eager>;
  using promise_type = PromiseT;
  struct PromiseHolder {
    PromiseHolder() = delete;
    PromiseHolder(PromiseT* p) : promise(p) {
      promise->register_owning_ptr(&promise);
    }
    ~PromiseHolder() {
      if (promise) promise->destroy();
    }
    PromiseT* operator->() { return promise; }
    PromiseT* promise;
  };

  Task() = default;
  Task(PromiseT* p) : prom(std::make_shared<PromiseHolder>(p)) {};

  // Copy and move support
  TaskT& operator=(const TaskT& o) {
    prom = o.prom;
    return *this;
  }
  TaskT& operator=(TaskT&& o) {
    *this = o;
    TaskT t{};
    o = t;
    return *this;
  }
  Task(const TaskT& o) { *this = o; };
  Task(TaskT&& o) noexcept { *this = std::move(o); };

  operator bool() const { return (bool)prom; };

  bool done() const { return promise().done(); }
  virtual void resume() { promise().resume(); }
  virtual void wait() { promise().wait(); }

  auto result() {
    this->wait();
    if constexpr (!std::is_same_v<T, void>) {
      return promise().result();
    }
  }

 private:
  mutable std::shared_ptr<PromiseHolder> prom;
  PromiseT& promise() const {
    assert(prom);
    return *(prom->promise);
  }
};

template <typename T>
using LazyTask = Task<T, false>;

} // namespace fenix::tasks

#endif // FENIX_TASKS_TASK_HPP
