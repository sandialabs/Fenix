#ifndef TASK_H
#define TASK_H
#include "fenix/tasks/task.h"
#include "fenix/logging/message_logging.h"

namespace fenix::logging {
class TaskT : public fenix::tasks::LazyTask<void> {
 public:
  using Parent = fenix::tasks::LazyTask<void>;
  using Parent::Parent;
  TaskT(const Parent& p) { Parent::operator=(p); }
  void resume() override {
    auto setting = scoped_logging(false);
    Parent::resume();
  }
  void wait() override {
    auto setting = scoped_logging(false);
    Parent::wait();
  }
};
} //namespace fenix::logging
#endif
