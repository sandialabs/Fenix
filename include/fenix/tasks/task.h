#ifndef FENIX_TASKS_TASK_H
#define FENIX_TASKS_TASK_H

#include <cassert>
#include <memory>

#include "promise.h"
#include "request.h"
#include "forward.h"

namespace fenix::tasks {

template <typename T, bool eager /*= true*/>
class Task {
 public:
  using PromiseT = Promise<T, eager>;
  using TaskT = Task<T, eager>;
  using promise_type = PromiseT;
  struct PromiseHolder {
    PromiseHolder() = delete;
    PromiseHolder(PromiseT* p) : promise(p) {}
    ~PromiseHolder() { promise->destroy(); }
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
    return promise().result();
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

#endif // FENIX_TASKS_TASK_H
