#include <mpi.h>
#include "fenix.hpp"
#include "fenix_ext.hpp"
#include "fenix/tasks/request.h"
#include "fenix/tasks/mpi.h"
#include "fenix/logging/comm_log.h"
#include "fenix/logging/util.h"
#include "fenix/logging/msg_log.h"

namespace fenix::logging {
using namespace fenix::tasks;

std::optional<CommLog> comm_log;

CommLog::CommLog(MPI_Comm& c, int m_max_regions)
  : comm(c), max_regions(m_max_regions) {
  init_mpi_records();
  regions.resize(max_regions);

  if (fenix::role() != fenix::INITIAL_RANK) {
    pending_reset = true;
  }
}

CommLog::CommLog(MPI_Comm& c, std::istream& i) : comm(c) {
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

  pending_reset = true;
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

bool CommLog::is_logging(MPI_Comm c) {
  return logging() && !pending_reset && !fenix_rt.ignore_errs && c == comm;
}

void CommLog::progress() {
  // Skip progress when error handling will be broken
  if (fenix_rt.ignore_errs || pending_reset) return;

  // Check if any other ranks are trying to form consistency with us
  int last_rank, rank = -1;
  do {
    last_rank = rank;

    int found;
    MPI_Status status;
    MPI_Iprobe(MPI_ANY_SOURCE, CONSISTENCY_TAG, comm, &found, &status);
    if (found) {
      auto& log = logs(rank = status.MPI_SOURCE);
      if (!log.task) {
        MLOG("Rank %d consistency initiated by rank %d\n", m_rank, rank);
        log.reply_consistency();
      }
      assert(log.task);
      log.task.resume();
    }
  } while (last_rank != rank);

  // Attempt to progress all active tasks
  for (int i = tasks.size() - 1; i >= 0; i--) {
    auto& task = tasks[i];
    task.resume();
    if (task.done()) tasks.erase(tasks.begin() + i);
  }
}

void CommLog::progress_through(TaskT t) {
  auto setting = scoped_logging(false);
  while (t && !t.done()) {
    t.resume();
    progress();
  }
}

Status CommLog::progress_through(MPI_Request* r) {
  Request request(r);
  while (!request.test()) progress();
  return request.result();
}

void CommLog::fenix_pre_recovery() {
  pending_reset = true;
  tasks.clear();
  for (auto& [rank, log] : rank_logs) {
    log.fenix_pre_recovery();
  }
  MLOG("%s completed fenix_pre_recovery\n", str().c_str());
}

void CommLog::reset_consistency(int region) {
  MLOG("%s resetting to region %d\n", str().c_str(), region);
  assert(m_rank == comm_rank(comm)); // No support for changing ranks
  assert(region >= -1);
  assert(tasks.empty());

  auto setting = scoped_logging(false);

#ifndef DNDEBUG
  MPI_Barrier(comm);
  // If we end up with tasks, pending_reset wasn't set correctly
  assert(tasks.empty());
  MLOG("%s completed reset_consistency barrier\n", str().c_str());
#endif

  if (region >= 0) active_region = region;
  for (auto& [rank, log] : rank_logs) {
    log.reset_consistency(region);
  }

  // TODO: set up regions as needed, call form_consistency as needed

  pending_reset = false;
  progress();
  MPI_Barrier(comm);
}

void CommLog::begin_region(int region) {
  assert(region >= 0);
  for (auto& [rank, log] : rank_logs) {
    log.begin_region(region);
  }
  active_region = region;
}

TaskT CommLog::form_consistency() {
  using namespace fenix::tasks::mpi;

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
        "%s region unknown to rank %d (between %d and %d)\n", str(true).c_str(),
        root, (it - 1)->str().c_str(), it->str().c_str()
      );
    } else if (!m_region.valid() || m_region.next != it->first) {
      fatal_print(
        "%s region unknown to rank %d (earliest=%d)\n", str(true).c_str(), root,
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

  completed_collective = latest_idx - 1;
  replay_collectives(earliest_idx);
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
  // TODO: Erase collective message logs within r
  assert(false);
}
void CommLog::replay_collectives(int start_idx) {
  // TODO: Do this
  assert(false);
}
} //namespace fenix::logging
