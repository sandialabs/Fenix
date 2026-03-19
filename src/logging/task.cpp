#include "fenix/logging/task.h"
#include "fenix/logging/comm_log.h"
#include "fenix_util.hpp"

namespace fenix::logging {
void TaskT::resume() {
  util::ScopedActiveMlog setting(FENIX_MLOG_NONE);
  Parent::resume();
}

void TaskT::wait() {
  util::ScopedActiveMlog setting(FENIX_MLOG_NONE);
  Parent::wait();
}
}
