#ifndef FENIX_TASKS_SUBTASK_H
#define FENIX_TASKS_SUBTASK_H

#include <coroutine>
#include <exception>
#include <memory>

namespace fenix::tasks {

template <typename T>
concept Subtaskable = requires(T t) {
  { t.done() } -> std::same_as<bool>;
  t.resume();
  t.wait();
  t.result();
};

class SubtaskBase {
 public:
  virtual ~SubtaskBase() = default;

  virtual bool done() const = 0;
  virtual void resume() const = 0;
  virtual void wait() const = 0;
};

namespace impl {
template <Subtaskable T>
struct SubtaskHolder {
  using U = std::remove_reference_t<T>;
  SubtaskHolder(T&& t) : task(std::make_shared<U>(std::move(t))) {}
  U* operator->() { return task.get(); }
  std::shared_ptr<U> task;
};
template <Subtaskable T>
struct SubtaskHolder<T&> {
  SubtaskHolder(T& t) : task(t) {};
  T* operator->() { return &task; }
  T& task;
};
} // namespace impl

template <Subtaskable T>
class Subtask : public SubtaskBase {
 public:
  using HolderT = impl::SubtaskHolder<T>;

  Subtask(T&& t) : task(std::forward<T>(t)) {};
  ~Subtask() = default;

  HolderT& operator->() const { return task; }

  bool done() const override { return task->done(); }
  void resume() const override { task->resume(); }
  void wait() const override {
    task->wait();
    assert(task->done());
  }
  auto result() const {
    assert(task->done());
    return task->result();
  }

  mutable HolderT task;
};

} // namespace fenix::tasks

#endif //FENIX_TASKS_SUBTASK_H
