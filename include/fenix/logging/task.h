#ifndef TASK_H
#define TASK_H
#include "fenix/tasks/task.h"

namespace fenix::logging {
class TaskT : public fenix::tasks::LazyTask<void> {
 public:
  using Parent = fenix::tasks::LazyTask<void>;
  using Parent::Parent;
  TaskT(const Parent& p) { Parent::operator=(p); }

  // Disabled message logging during internal message log tasks
  void resume() override;

  // Disabled message logging during internal message log tasks
  void wait() override;
};
} //namespace fenix::logging
#endif
