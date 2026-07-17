#include <sstream>
#include "fenix.hpp"
#include "fenix_util.hpp"
#include "fenix/logging/message_logging.h"
#include "fenix/logging/serialize.h"
#include "fenix/logging/util.h"
#include "fenix/logging/comm_log.h"

namespace fenix::mlog {
using namespace fenix::logging;

namespace impl {

std::shared_ptr<CommLog> search_mlog(int id) {
  auto iter = fenix_rt.mlogs.find(id);
  if (iter == fenix_rt.mlogs.end()) return {};
  else return iter->second;
}

std::shared_ptr<CommLog> find_mlog(int id, std::source_location loc) {
  auto ret = search_mlog(id);
  if (!ret) FENIX_THROW_FROM(FENIX_ERROR_INVALID_MLOGID, loc);
  return ret;
}

} // namespace impl

using namespace impl;

int create(int mlog_id, MPI_Comm& comm, int depth) {
  FENIX_CPP_API_BEGIN
  if (!fenix_rt.mpi_overloads_linked)
    FENIX_THROW(FENIX_ERROR_MLOG_LIBRARY_UNAVAILABLE);
  auto mlog             = std::make_shared<CommLog>(comm, depth);
  auto [iter, inserted] = fenix_rt.mlogs.try_emplace(mlog_id, mlog);
  if (!inserted) FENIX_THROW(FENIX_ERROR_MLOG_EXISTS);
  else fenix_rt.mlog_order.push_back(mlog_id);
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

int activate(int mlog_id) {
  FENIX_CPP_API_BEGIN
  // Always set to no mlog first, so errors leave us in a defined state
  fenix_rt.active_mlog    = nullptr;
  fenix_rt.active_mlog_id = FENIX_MLOG_NONE;
  if (mlog_id != FENIX_MLOG_NONE) {
    fenix_rt.active_mlog    = find_mlog(mlog_id);
    fenix_rt.active_mlog_id = mlog_id;
  }
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

int begin_region(int mlog_id, int region_id) {
  FENIX_CPP_API_BEGIN
  find_mlog(mlog_id)->begin_region(region_id);
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

int activate(int mlog_id, int region_id) {
  FENIX_CPP_API_BEGIN
  if (mlog_id == FENIX_MLOG_NONE) FENIX_THROW(FENIX_ERROR_INVALID_MLOGID);
  int ret = activate(mlog_id);
  if (ret == FENIX_SUCCESS) ret = begin_region(mlog_id, region_id);
  return ret;
  FENIX_CPP_API_END
}

int active() { return finalized() ? FENIX_MLOG_NONE : fenix_rt.active_mlog_id; }

int sync(int mlog_id, int region_id) {
  FENIX_CPP_API_BEGIN
  find_mlog(mlog_id)->reset_consistency(region_id);
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

int create_data_member(int mlog_id, int group_id, int member_id) {
  FENIX_CPP_API_BEGIN
  return data::member_create(
    group_id, member_id, nullptr, FENIX_RESIZEABLE, MPI_BYTE,
    [mlog_id](
      std::iostream& strm, int direction, void* buf, int offset, int count
    ) {
      if (direction == FENIX_SERIALIZE) {
        fenix_assert(offset == 0 && count == FENIX_RESIZEABLE && !buf);
        find_mlog(mlog_id)->serialize(strm);
      } else {
        fenix_assert(offset == 0 && !buf);
        auto mlog     = find_mlog(mlog_id);
        auto new_mlog = std::make_shared<CommLog>(mlog->comm, strm);

        fenix_rt.mlogs[mlog_id] = new_mlog;
        if (fenix_rt.active_mlog == mlog) fenix_rt.active_mlog = new_mlog;
      }
    }
  );
  FENIX_CPP_API_END
}

int mlog_delete(int mlog_id) {
  FENIX_CPP_API_BEGIN
  if (fenix_rt.active_mlog_id == mlog_id) {
    fenix_rt.active_mlog    = nullptr;
    fenix_rt.active_mlog_id = FENIX_MLOG_NONE;
  }
  fenix_rt.mlogs.erase(mlog_id);
  for (int i = 0; i < fenix_rt.mlog_order.size(); i++) {
    if (fenix_rt.mlog_order[i] == mlog_id) {
      fenix_rt.mlog_order.erase(fenix_rt.mlog_order.begin() + i);
      break;
    }
  }
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

} //namespace fenix::mlog
