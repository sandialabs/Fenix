#include <algorithm>

#include <mpi.h>
#include "fenix.hpp"
#include "fenix_ext.hpp"
#include "fenix_util.hpp"
#include "fenix/mpixx/request.hpp"
#include "fenix/mpixx/tasks.hpp"
#include "fenix/logging/message_logging.h"
#include "fenix/logging/comm_log.h"
#include "fenix/logging/util.h"

namespace fenix::logging {
using namespace fenix::tasks;

std::optional<CommLog> comm_log;

CommLog::CommLog(MPI_Comm& c, int m_max_regions)
  : comm(c), m_rank(mpixx::comm_rank(c)), max_regions(m_max_regions) {
  init_mpi_records();
  regions.resize(max_regions);
}

CommLog::CommLog(MPI_Comm& c, std::istream& i)
  : comm(c), m_rank(mpixx::comm_rank(c)) {
  using namespace serialize;
  init_mpi_records();
  int rank = read<int>(i);
  if (rank != m_rank) {
    fatal_print("Error: rank %d recovering as rank %d\n", m_rank, rank);
  }

  read(i, max_regions);
  read(i, active_region);
  read(i, regions);

  int n_logs = read<int>(i);
  for (int l = 0; l < n_logs; l++) {
    read(i, rank);
    auto [it, emplaced] = rank_logs.try_emplace(rank, *this, rank, i);
    assert(emplaced);
  }
}

void CommLog::serialize(std::ostream& o) {
  using namespace serialize;
  write(o, m_rank);
  write(o, max_regions);
  write(o, active_region);
  write(o, regions);

  int n_logs = 0;
  for (auto& [rank, log] : rank_logs) {
    if (log.cur_region.valid()) n_logs++;
  }
  write(o, n_logs);

  for (auto& [rank, log] : rank_logs) {
    if (log.cur_region.valid()) {
      write(o, rank);
      write(o, log);
    }
  }
}

RankLog& CommLog::logs(int r) {
  assert(r >= 0);
  return rank_logs.try_emplace(r, *this, r).first->second;
}

void CommLog::progress() {
  if (tasks.empty()) return;

  // Attempt to progress all active tasks
  for (int i = tasks.size() - 1; i >= 0; i--) {
    auto& task = tasks[i];
    task.resume();
    if (task.done()) tasks.erase(tasks.begin() + i);
  }
}

void CommLog::progress_through(TaskT t) {
  util::ScopedActiveMlog setting(FENIX_MLOG_NONE);
  while (t && !t.done()) {
    t.resume();
    progress();
  }
}

mpixx::Status CommLog::progress_through(MPI_Request* r) {
  int complete = 0;
  mpixx::Status ret;
  while (!complete) {
    ret = PMPI_Test(r, &complete, ret);
    progress();
  }
  return ret;
}

int CommLog::begin(CollectiveLogHolder&& new_op) {
  fenix_assert(!active_op);
  active_op = std::move(new_op);

  int op_idx = active_op->idx();
  int ret    = MPI_SUCCESS;

  // For now, we have a very simple approach of just replaying all collectives
  // that any ranks need. So, no fancy logic needed when recovering from faults.
  while (true) {
    try {
      util::ScopedDefaultRuntimeOptions settings;
      fenix_assert(!task);
      fenix_assert(region().valid());
      fenix_assert(region() == active_region);
      fenix_assert(active_op);
      fenix_assert(active_op->idx() == op_idx);

      ret = active_op->begin(comm);
      fenix_assert(ret == MPI_SUCCESS);
    } catch (const CommException& e) {
      if (fenix_rt.settings.mlog_recovery == MANUAL) {
        active_op = CollectiveLogHolder();
        util::resume_application();
        return e.mpi_err;
      } else {
        continue;
      }
    }
    break;
  }

  collectives.insert(collectives.end(), std::move(active_op));
  fenix_assert(!active_op);

  return ret;
}

void CommLog::fenix_pre_recovery() {
  tasks.clear();
  for (auto& [rank, log] : rank_logs) {
    log.fenix_pre_recovery();
  }
  MLOG("%s completed fenix_pre_recovery\n", str().c_str());
}

void CommLog::reset_consistency(int target_region) {
  MLOG("%s resetting to region %d\n", str().c_str(), target_region);
  assert(m_rank == mpixx::comm_rank(comm)); // No support for changing ranks
  assert(target_region >= -1);
  assert(tasks.empty());
  util::ScopedActiveMlog setting(FENIX_MLOG_NONE);

  // Call reset_consistency for all point-to-point logs
  if (target_region >= 0) active_region = target_region;
  for (auto& [rank, log] : rank_logs) {
    log.reset_consistency(target_region);
  }

  // Reset our collective region information, if needed
  if (region().valid() && target_region != -1 && region() > target_region) {
    // Erase any CRegions after the reset region
    auto it = std::upper_bound(regions.begin(), regions.end(), target_region);
    erase_regions(it, regions.end());
    fenix_assert(
      !region().valid() || region() == target_region,
      "Recovering to an undefined region between two defined regions!"
    );
  } else if (region().valid() && target_region > region()) {
    // Assume we are recovering into the very next region
    append_region({target_region, region().next});
  } else if (region().valid() && region() == target_region) {
    // We're recovering to the beginning of this region, so erase prior logs
    erase_logs(region());
    region().next = region().first;
  }

  // If we have any collective logs, begin forming collective consistency
  if (region().valid() && !region().fresh()) {
    task = form_consistency();
    tasks.push_back(task);
  }

  // Progress through all of the consistency tasks we know about
  do {
    progress();
    detect_incoming_consistency_request();
  } while (!tasks.empty());

  // Once locally completed, enter a barrier to wait for global completion
  MPI_Request req;
  int complete;
  MPI_Ibarrier(comm, &req);
  do {
    MPI_Test(&req, &complete, MPI_STATUS_IGNORE);
    // Make sure to contribute to any newly discovered tasks from remote ranks
    detect_incoming_consistency_request();
    progress();
  } while (!complete);

  // Since all ranks completed anything they started, all ranks should be left
  // with no tasks remaining.
  assert(tasks.empty());

  // Wait until after barrier has completed to begin replaying collectives
  replay_collectives(completed_collective_all + 1);
}

void CommLog::detect_incoming_consistency_request() {
  MPI_Status s;
  int found;

  if (!task) {
    MPI_Iprobe(MPI_ANY_SOURCE, COLLECTIVE_CONSISTENCY_TAG, comm, &found, &s);
    if (found) {
      task = form_consistency();
      tasks.push_back(task);
    }
  }

  MPI_Iprobe(MPI_ANY_SOURCE, CONSISTENCY_TAG, comm, &found, &s);
  if (found && !logs(s.MPI_SOURCE).task) {
    MLOG("Rank %d consistency initiated by rank %d\n", m_rank, s.MPI_SOURCE);
    logs(s.MPI_SOURCE).reply_consistency();
  }
}

void CommLog::begin_region(int region_id) {
  assert(region_id >= 0);
  for (auto& [rank, log] : rank_logs) {
    log.begin_region(region_id);
  }
  active_region = region_id;
  if (region() > region_id) {
    // Fine if no collectives have been run in region() yet, because remote
    // ranks may have told us about region() before we reached it (while forming
    // consistency)
    if (!region().empty()) {
      fatal_print("Attempt to begin_region before current region");
    }
    return;
  } else if (region() == region_id) {
    // Again, fine if no collectives have run yet
    fenix_assert(region().valid());
    if (!region().empty()) {
      fatal_print("Duplicate begin_region");
    }
  } else {
    if (region().valid()) append_region({region_id, region().next});
    else append_region({region_id, 0});
  }
}

TaskT CommLog::form_consistency() {
  using namespace fenix::mpixx;

  int n_ranks    = mpixx::comm_size(comm);
  int left_rank  = (m_rank + n_ranks - 1) % n_ranks;
  int right_rank = (m_rank + 1) % n_ranks;
  // This just serves as a notification to other ranks that we do need to
  // perform collectives consistency forming. Every rank sends to left with
  // COLLECTIVE_CONSISTENCY_TAG so an iprobe can detect this
  co_await sendrecv(
    m_rank,     left_rank,  COLLECTIVE_CONSISTENCY_TAG,
    right_rank, right_rank, COLLECTIVE_CONSISTENCY_TAG, comm
  );

  auto& m_region = region();

  // Figure out which rank has the most up-to-date information
  Indexed<int> latest_idx_pair;
  Indexed<int> my_idx_pair = {.value = m_region.next, .index = m_rank};
  co_await allreduce(my_idx_pair, latest_idx_pair, MPI_MAXLOC, comm);
  int latest_idx = latest_idx_pair.value, root = latest_idx_pair.index;

  MLOG("%s max coll idx %d on rank %d\n", str(true).c_str(), latest_idx, root);
  assert(latest_idx >= 0);

  // Broadcast the most up-to-date information to everyone
  std::vector<CRegion> latest_regions(max_regions);
  if (root == m_rank) latest_regions = regions;
  co_await bcast(latest_regions, root, comm);

  // Ensure consistency with most up-to-date information
  if (!m_region.valid()) m_region.id = active_region;
  auto it =
    std::lower_bound(latest_regions.begin(), latest_regions.end(), m_region);
  if (it == latest_regions.end()) {
    // We are past root's latest region
    if (!m_region.valid() || m_region.fresh()) {
      // Assume latest_idx is the border between latest region and ours
      m_region.first = m_region.next = latest_idx;
    } else if (m_region.first != latest_idx) {
      fatal_print(
        "%s region information conflicts with rank %d!\n", str(true).c_str(),
        root
      );
    }
  } else if (*it != m_region) {
    // We are before root's earliest region
    if (it != latest_regions.begin() && (it - 1)->valid()) {
      fatal_print(
        "%s region unknown to rank %d (between %s and %s)\n", str(true).c_str(),
        root, (it - 1)->str().c_str(), it->str().c_str()
      );
    } else if (!m_region.valid() || m_region.next != it->first) {
      fatal_print(
        "%s region unknown to rank %d (earliest=%s)\n", str(true).c_str(), root,
        it->str().c_str()
      );
    } else {
      // We're in an earlier region than root has, but we know we're at the
      // right index to match up once the user next calls begin_region. Append
      // now so we can catch it if they try any other logged collectives before
      // beginning the correct region
      append_region({it->id, m_region.next});
    }
  } else {
    // We found our region in root's information
    if (!m_region.valid() || m_region.fresh()) {
      m_region.first = m_region.next = it->first;
    } else if (m_region.first != it->first) {
      fatal_print(
        "%s region information conflicts with rank %d's %s\n",
        str(true).c_str(), root, it->str().c_str()
      );
    }
  }

  // Figure out which operations need replaying
  int earliest_idx;
  co_await allreduce(m_region.next, earliest_idx, MPI_MIN, comm);

  completed_collective_any = latest_idx - 1;
  completed_collective_all = earliest_idx - 1;
  task                     = {};
}

void CommLog::append_region(const CRegion& r) {
  assert(region() < r);
  erase_logs(regions.front());
  for (int i = 0; i < regions.size() - 1; i++) {
    regions[i] = regions[i + 1];
  }
  region() = r;
}

void CommLog::erase_logs(const CRegion& r) {
  if (!r.valid()) return;
  collectives.erase(
    collectives.lower_bound(r.first), collectives.lower_bound(r.next)
  );
}
void CommLog::erase_regions(
  std::vector<CRegion>::iterator begin, std::vector<CRegion>::iterator end
) {
  for (auto it = begin; it < end; it++) erase_logs(*it);
  while (begin != regions.begin()) *(--end) = *(--begin);
  while (end != regions.begin()) *(--end) = CRegion();
}
void CommLog::replay_collectives(int start_idx) {
  fenix_assert(region().valid());
  if (start_idx >= region().next) return;

  int prev_idx = start_idx - 1;
  for (auto i = collectives.find(start_idx); i != collectives.end(); i++) {
    fenix_assert(
      (*i)->idx() == prev_idx + 1, "Rank %d missing collective %d (next = %d)",
      m_rank, prev_idx + 1, (*i)->idx()
    );
    prev_idx++;
    (*i)->replay(comm);
  }
}
} //namespace fenix::logging
