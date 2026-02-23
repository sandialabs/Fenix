#ifndef COMM_LOG_H
#define COMM_LOG_H

#include <map>
#include <vector>
#include <cassert>
#include <istream>
#include <ostream>
#include <optional>

#include <mpi.h>

#include "fenix_util.hpp"
#include "fenix/tasks/request.h"
#include "fenix/logging/message_logging.h"
#include "fenix/logging/task.h"
#include "fenix/logging/rank_log.h"
#include "fenix/logging/collective_log_holder.h"

namespace fenix::logging {

struct CRegion {
  int id = -1, first = -1, next = -1;
  CRegion() = default;
  CRegion(int m_id) : id(m_id) {};
  CRegion(int m_id, int idx) : id(m_id), first(idx), next(idx) {}
  auto operator<=>(const CRegion& o) const { return id <=> o.id; }
  auto operator==(const CRegion& o) const { return id == o.id; }
  auto operator<=>(const int& i) const { return id <=> i; }
  auto operator==(const int& i) const { return id == i; }
  bool valid() const { return id >= 0 && first >= 0 && next >= 0; }
  bool empty() const { return valid() && first == next; }
  bool fresh() const { return empty() && first == 0; }
  std::string str() const {
    return "Region " + std::to_string(id) + " [" + std::to_string(first) + "," +
           std::to_string(next) + ")";
  }
};

struct CommLog {
  CommLog(MPI_Comm& c, int m_max_regions = 2);
  CommLog(MPI_Comm& c, std::istream& i);
  void serialize(std::ostream& o);

  MPI_Comm& comm;
  const int m_rank = util::comm_rank(comm);
  int max_regions;
  int active_region = 0;

  std::map<int, RankLog> rank_logs;
  std::vector<TaskT> tasks;

  std::vector<CRegion> regions;
  std::set<CollectiveLogHolder, std::less<>> collectives;
  CollectiveLogHolder active_op;
  // Collective index every rank has successfully completed, as of last reset
  int completed_collective_all = -1;
  // As above, but index any rank has successfully completed
  int completed_collective_any = -1;
  TaskT task;

  RankLog& logs(int r);
  RankLog& operator[](int r) { return logs(r); }

  // Attempt progress on each task
  void progress();
  // Progress pending tasks and this one until this task completes
  void progress_through(TaskT t);
  fenix::tasks::Status progress_through(MPI_Request* r);

  bool is_logging(MPI_Comm c);

  int send(const void* b, int n, MPI_Datatype d, int dst, int t) {
    return logs(dst).send(b, n, d, t);
  }
  int irecv(void* b, int n, MPI_Datatype d, int src, int t, MPI_Request* r) {
    return logs(src).irecv(b, n, d, t, r);
  }

  // Return from logged collective calls. MPI return code and the log
  using CollectiveResult =
    std::pair<int, std::reference_wrapper<const CollectiveLogHolder>>;

  // Begin a logged collective MPI Function, return the log
  template <auto MPIFunction, typename... Args>
  int begin(Args... args) {
    using LogT = mpi_log_t<MPIFunction>;

    if (!region().valid()) append_region({active_region, 0});
    return begin(
      CollectiveLogHolder::template create<LogT>(args..., region().next++)
    );
  }

  void fenix_pre_recovery();
  void reset_consistency(int checkpoint_id);

  void begin_region(int region);
  CRegion& region() { return regions.back(); }
  const CRegion& region() const { return regions.back(); }

  std::string str(bool with_region = false) const {
    return "Rank " + std::to_string(m_rank) +
           " (active=" + std::to_string(active_region) + ")" +
           (with_region ? " " + region().str() : "");
  }

 private:
  // Iprobe MPI for any other rank trying to form consistency
  void detect_incoming_consistency_request();
  TaskT form_consistency();
  void replay_collectives(int start_idx);
  void append_region(const CRegion& r);
  void erase_logs(const CRegion& r);
  void erase_regions(
    std::vector<CRegion>::iterator begin, std::vector<CRegion>::iterator end
  );

  // Returns reference to logged op in collectives set
  int begin(CollectiveLogHolder&& collective_op);
};

extern std::optional<CommLog> comm_log;

} //namespace fenix::logging

#endif
