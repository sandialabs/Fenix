#ifndef MESSAGE_LOGGING_H
#define MESSAGE_LOGGING_H
#include <istream>
#include <ostream>
#include <cassert>

#include <mpi.h>
#include "fenix_ext.hpp"
#include "fenix/logging/comm_log.h"

#define CONSISTENCY_TAG 22234652
#define COLLECTIVE_CONSISTENCY_TAG 22234653

namespace fenix::mlog {
using namespace fenix::logging;
}
namespace fenix::logging {
using namespace fenix::mlog;
}

namespace fenix::mlog::impl {

// Search for the specified log, returning an empty ptr if not found
std::shared_ptr<CommLog> search_mlog(int id);
// As search_log, but throws if log not found
std::shared_ptr<CommLog> find_mlog(
  int id, std::source_location loc = std::source_location::current()
);

// Small helper
inline bool is_logging(MPI_Comm c) {
  return fenix_rt.active_mlog && fenix_rt.active_mlog->comm == c;
}

} //namespace fenix::mlog::impl

#endif
