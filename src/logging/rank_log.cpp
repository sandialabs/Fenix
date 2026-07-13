#include <algorithm>

#include "fenix.hpp"
#include "fenix/tasks/mpi.hpp"
#include "fenix/logging/message_logging.h"
#include "fenix/logging/util.h"
#include "fenix/logging/rank_log.h"
#include "fenix/logging/comm_log.h"

namespace fenix::logging {
RankLog::RankLog(CommLog& m_comm_log, int m_rank)
  : comm_log(m_comm_log), rank(m_rank), regions(comm_log.max_regions) {}

RankLog::RankLog(CommLog& m_comm_log, int m_rank, std::istream& i)
  : RankLog(m_comm_log, m_rank) {
  using namespace serialize;
  read(i, sends);
  read(i, already_sent);
  read(i, regions);
  assert(regions.size() == comm_log.max_regions);
  assert(cur_region.valid());
}

void RankLog::serialize(std::ostream& o) const {
  using namespace serialize;
  assert(cur_region.valid());
  assert(!active_irecv);
  write(o, sends);
  write(o, already_sent);
  write(o, regions);
}

std::string RankLog::str() const {
  return comm_log.str() + " Log " + std::to_string(rank) + " " +
    cur_region.str();
}

void RankLog::reset_consistency(int target_region) {
  assert(!task);
  assert(target_region >= -1);
  assert(target_region < 0 || !active_irecv);

  // Only form consistency if we have a valid region, otherwise ignore until we
  // have a message to/from them or they initiate forming consistency
  if (!cur_region.valid()) return;

  if (0 <= target_region && target_region < cur_region) {
    // Erase future regions
    auto it = std::upper_bound(regions.begin(), regions.end(), target_region);
    erase_regions(it, regions.end());

    // We may be recovering to something older than we've still got logged
    if (!cur_region.valid()) return;

    // This is an application logic error
    fenix_assert(
      cur_region == target_region,
      "Recovering to an undefined region between two defined regions!"
    );
  } else if (target_region > cur_region) {
    // Assume we are recovering into the very next region
    append_region({target_region, cur_region});
  }

  if (cur_region == target_region) {
    // We're recovering to the beginning of this region, so erase prior msgs
    erase_sends(cur_region);
    next_send = cur_region.send.first;
    next_recv = cur_region.recv.first;
  }

  // If we are initiating a consistency task, we must have a valid region
  assert(cur_region.valid());
  assert(cur_region == target_region || target_region == -1);
  comm_log.tasks.push_back(task = form_consistency());
}

void RankLog::reply_consistency() {
  comm_log.tasks.push_back(task = form_consistency());
}

void RankLog::check_consistent(std::vector<Region>& a, std::vector<Region>& b) {
  // This function only to check valid regions
  assert(a.back().valid() && b.back().valid());
  if (a.back() < b.back()) return check_consistent(b, a);

  auto it = std::lower_bound(a.begin(), a.end(), b.back());
  assert(it != a.end());

  auto& ra = *it;
  auto& rb = b.back();

  if (rb != ra) {
    auto& remote_region = &a == &regions ? b.back() : a.back();
    fatal_print(
      "%s remote's %s unknown (increase max regions?)\n", str().c_str(),
      remote_region.str().c_str()
    );
  }

  // If start indices match, we're good
  if (ra.send.first == rb.recv.first && ra.recv.first == rb.send.first) return;

  // Otherwise if either thinks no messages have ever been sent, we can just
  // update them with the correct indices
  if (rb.fresh()) {
    rb.send.first = rb.send.next = ra.recv.first;
    rb.recv.first = rb.recv.next = ra.send.first;
  } else if (ra.fresh() && ra == a.back()) {
    ra.send.first = ra.send.next = rb.recv.first;
    ra.recv.first = ra.recv.next = rb.send.first;
  } else {
    fatal_print(
      "%s conflicting regions %s and %s\n", str().c_str(), ra.str().c_str(),
      rb.str().c_str()
    );
  }
}

void RankLog::recover_invalid(std::vector<Region>& a, std::vector<Region>& b) {
  if (!a.back().valid()) {
    assert(b.back().valid());
    return recover_invalid(b, a);
  }
  assert(b.back() >= 0);

  auto it = std::lower_bound(a.begin(), a.end(), b.back());

  bool missing = it == a.end() || (*it != b.back() && !it->fresh());
  if (missing) {
    if (b.back() == cur_region) {
      fatal_print(
        "%s missing logs for partner in %s\n", str().c_str(),
        a.back().str().c_str()
      );
    } else {
      fatal_print(
        "%s partner in %s missing my logs\n", str().c_str(),
        b.back().str().c_str()
      );
    }
  }

  auto& ra = *it;
  auto& rb = b.back();

  if (ra != rb) {
    assert(ra.fresh());
    rb = Region(ra.id, 0, 0);
  } else {
    rb.send.first = rb.send.next = ra.recv.first;
    rb.recv.first = rb.recv.next = ra.send.first;
  }
}

TaskT RankLog::form_consistency() {
  using namespace fenix::tasks::mpi;

  if (!cur_region.valid()) {
    assert(!active_irecv);
    // Still invalid, but let them know what my active region is
    cur_region = Region(comm_log.active_region);
  }

  MLOG("%s started consistency task\n", str().c_str());

  std::vector<Region> remote_regions(regions.size());
  // clang-format off
  co_await sendrecv(
           regions, rank, CONSISTENCY_TAG,
    remote_regions, rank, CONSISTENCY_TAG, comm_log.comm
  );
  // clang-format on

  Region& cur_remote = remote_regions.back();
  assert(cur_remote.valid() || cur_region.valid());

  if (cur_remote.valid() && cur_region.valid()) {
    check_consistent(regions, remote_regions);
  } else {
    recover_invalid(regions, remote_regions);
  }
  assert(cur_region.valid());
  assert(cur_remote.valid());

  already_sent = cur_remote.recv.next - 1;
  replay_messages();

  MLOG("%s finished consistency task\n", str().c_str());
  task = TaskT();
  co_return;
}

void RankLog::replay_messages() {
  auto it = sends.lower_bound(already_sent + 1);
  assert(
    (it != sends.end() || next_send - 1 <= already_sent) &&
    "Error: Rank missing messages needed for replay!"
  );

  int prev_idx = already_sent;
  for (; it != sends.end(); prev_idx = (it++)->idx()) {
    assert(
      it->idx() == prev_idx + 1 &&
      "Error: Rank missing messages needed for replay!"
    );
    MLOG("%s replaying %s\n", str().c_str(), it->str().c_str());

    it->isend(rank, comm_log.comm);
  }

  //Re-activate any receives not yet completed
  if (active_irecv) {
    MLOG("%s replaying %s\n", str().c_str(), active_irecv.str().c_str());
    active_irecv.irecv(rank, comm_log.comm);
  }
}

void RankLog::ensure_consistency() {
  if (task) {
    fatal_print(
      "%s attempting logged action while forming consistency!\n", str().c_str()
    );
  }
  if (!cur_region.valid()) {
    append_region({comm_log.active_region, 0, 0});
  }
  fenix_assert(cur_region >= comm_log.active_region);
}

const SendLog& RankLog::log_send(const void* b, int n, MPI_Datatype d, int t) {
  ensure_consistency();
  if (cur_region != comm_log.active_region) {
    fatal_print("%s send unexpected until region active\n", str().c_str());
  }
  fenix_assert(sends.empty() || *(--sends.end()) == next_send - 1);
  return *sends.emplace_hint(sends.end(), b, n, d, t, next_send++);
}

int RankLog::send(const void* b, int n, MPI_Datatype d, int t) {
  auto& log = log_send(b, n, d, t);
  if (log <= already_sent) {
    MLOG(
      "%s skipping %s (skipping past %d)\n", str().c_str(), log.str().c_str(),
      already_sent
    );
    return MPI_SUCCESS;
  }
  try {
    int ret = log.isend(rank, comm_log.comm);
    if (ret == MPI_SUCCESS) ret = comm_log.progress_through(log.req());
    return ret;
  } catch (const fenix::CommException& e) {
    if (fenix_rt.settings.mlog_recovery == MANUAL) throw;
    else return MPI_SUCCESS;
  }
}

int RankLog::irecv(void* b, int n, MPI_Datatype d, int t, MPI_Request* r) {
  assert(!active_irecv);
  ensure_consistency();
  if (cur_region != comm_log.active_region) {
    fatal_print("%s recv unexpected until region active\n", str().c_str());
  }

  active_irecv = {b, n, d, t, r};
  try {
    return active_irecv.irecv(rank, comm_log.comm);
  } catch (const fenix::CommException& e) {
    if (fenix_rt.settings.mlog_recovery != MANUAL) {
      ensure_consistency();
      return MPI_SUCCESS;
    } else {
      active_irecv.reset();
      throw;
    }
  }
}

fenix::tasks::Status RankLog::wait(MPI_Request* r) {
  fenix_assert(r != NULL);
  fenix_assert(*r != MPI_REQUEST_NULL);
  fenix_assert(active_irecv);
  fenix_assert(r == active_irecv);

  fenix::tasks::Status ret;

  while (true) {
    try {
      fenix_assert(cur_region.valid());
      fenix_assert(r == active_irecv);
      fenix_assert(*r != MPI_REQUEST_NULL);

      ret = comm_log.progress_through(r);
    } catch (const fenix::CommException& e) {
      if (fenix_rt.settings.mlog_recovery != MANUAL) continue;
      throw;
    }
    break;
  }

  next_recv++;
  active_irecv.reset();

  return ret;
}

void RankLog::begin_region(int region_id) {
  fenix_assert(region_id >= 0);

  if (cur_region > region_id) {
    // Fine if no messages have been send in cur_region yet, because the remote
    // rank may have told us about cur_region before we reached it (while
    // forming consistency)
    if (!cur_region.empty()) {
      fatal_print("Attempt to begin_region before current region");
    }
    return;
  } else if (cur_region == region_id) {
    // Allowed when no messages have been sent in the region yet,
    // which lets us create the region when recovering without making
    // the user check if they just recovered when calling begin_region
    fenix_assert(cur_region.valid());
    if (!cur_region.empty()) {
      fatal_print("Duplicate begin_region");
    }
  } else {
    if (cur_region.valid()) {
      append_region({region_id, cur_region});
    } else {
      append_region({region_id, 0, 0});
    }
  }

  assert(cur_region.valid());
  assert(cur_region.empty());
  assert(cur_region == region_id);

  MLOG("%s beginning region\n", str().c_str());
}

void RankLog::append_region(const Region& r) {
  assert(cur_region < r);
  erase_sends(regions.front());
  for (int i = 0; i < regions.size() - 1; i++) {
    regions[i] = regions[i + 1];
  }
  cur_region = r;
}

void RankLog::erase_sends(const Region& r) {
  if (!r.valid()) return;
  sends.erase(sends.lower_bound(r.send.first), sends.lower_bound(r.send.next));
}

void RankLog::erase_regions(
  std::vector<Region>::iterator begin, std::vector<Region>::iterator end
) {
  for (auto it = begin; it < end; it++) erase_sends(*it);
  while (begin != regions.begin()) *(--end) = *(--begin);
  while (end != regions.begin()) *(--end) = Region();
}
} //namespace fenix::logging
