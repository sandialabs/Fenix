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

int stage(int mlog_id, int group_id, int member_id) {
  FENIX_CPP_API_BEGIN
  using namespace fenix::data;
  auto mlog  = find_mlog(mlog_id);
  auto group = find_group(group_id);

  auto member = group->search_member(member_id);
  if (!member) {
    member_create(group_id, member_id, nullptr, FENIX_RESIZEABLE, MPI_BYTE);
    member = group->find_member(member_id);
  } else if (member->current_count != FENIX_RESIZEABLE) {
    FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  } else if (member->datatype_size != 1) {
    FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  }

  std::stringstream o;
  mlog->serialize(o);

  void* ptr = (void*)o.view().data();

  int flag;
  Fenix_Data_member_attr_set(
    group_id, member_id, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
    (void*)o.view().data(), &flag
  );

  return member_stage(
    group_id, member_id, DataSubset(static_cast<int>(o.tellp()) - 1)
  );
  FENIX_CPP_API_END
}

int lrestore(int mlog_id, int group_id, int member_id, int time_stamp) {
  FENIX_CPP_API_BEGIN
  using namespace fenix::data;
  auto mlog  = find_mlog(mlog_id);
  auto group = find_group(group_id);

  auto member = group->find_member(member_id);
  if (member->current_count != FENIX_RESIZEABLE) {
    FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  } else if (member->datatype_size != 1) {
    FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  }

  DataSubset subset;
  int ret = group->member_lrestore(member_id, nullptr, 0, time_stamp, subset);
  if (ret != FENIX_SUCCESS) {
    throw RuntimeException(ret, "fenix::mlog::lrestore failed");
  }

  // Initialize a long enough buffer string
  int len = subset.max_count();
  std::string buf(static_cast<std::string::size_type>(len), ' ');
  ret = group->member_lrestore(member_id, &buf[0], len, time_stamp, subset);
  if (ret != FENIX_SUCCESS) {
    throw RuntimeException(ret, "fenix::mlog::lrestore failed");
  }

  std::istringstream i(std::move(buf));
  assert(i.view().size() == len);

  auto new_mlog           = std::make_shared<CommLog>(mlog->comm, i);
  fenix_rt.mlogs[mlog_id] = new_mlog;
  if (fenix_rt.active_mlog == mlog) fenix_rt.active_mlog = new_mlog;

  return FENIX_SUCCESS;
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
