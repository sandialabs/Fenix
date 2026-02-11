#ifndef RANK_LOG_H
#define RANK_LOG_H
#include <vector>

#include <mpi.h>

#include "fenix/logging/task.h"
#include "fenix/logging/ops/send_log.h"
#include "fenix/logging/ops/irecv_log.h"

namespace fenix::logging {
struct MsgRange {
  int first = -1, next = -1;
  MsgRange() = default;
  explicit MsgRange(int m_first) : first(m_first), next(first) {}
  // Valid ranges have non-negative indices
  bool valid() const { return 0 <= first && first <= next; }
  // Empty ranges are valid with overlapping indices.
  bool empty() const { return 0 <= first && first == next; }
  // Fresh ranges are empty at 0
  bool fresh() const { return 0 == first && first == next; }
  std::string str() const {
    return "[" + std::to_string(first) + "," + std::to_string(next) + ")";
  }
};

struct Region {
  int id = -1;
  MsgRange send, recv;

  // Valid regions have a non-negative ID and valid ranges
  bool valid() const { return id >= 0 && send.valid() && recv.valid(); }
  // Empty regions are valid with empty ranges
  bool empty() const { return id >= 0 && send.empty() && recv.empty(); }
  // Fresh regions are valid with fresh ranges
  bool fresh() const { return id >= 0 && send.fresh() && recv.fresh(); }

  Region() = default;
  explicit Region(int m_id) : id(m_id) {}
  Region(int m_id, int first_send, int first_recv)
    : id(m_id), send(first_send), recv(first_recv) {}
  Region(int m_id, const Region& o) : Region(m_id, o.send.next, o.recv.next) {
    assert(m_id > o.id);
  }

  auto operator<=>(const Region& o) const { return id <=> o.id; }
  auto operator<=>(const int& i) const { return id <=> i; }
  auto operator==(const Region& o) const { return id == o.id; }
  auto operator==(const int& i) const { return id == i; }
  std::string str() const {
    return "Region " + std::to_string(id) + " (send:" + send.str() +
           ",recv:" + recv.str() + ")";
  }
};

struct CommLog;

struct RankLog {
  RankLog(CommLog& m_comm_log, int m_rank);
  RankLog(CommLog& m_comm_log, int m_rank, std::istream& i);
  void serialize(std::ostream& o) const;

  CommLog& comm_log;
  const int rank;
  TaskT task;

  IrecvLog active_irecv;

  void begin_region(int region_id);

  // Called when the user resets consistency
  void reset_consistency(int target_region);
  // Called when a remote rank unexpectedly tries to form consistency with us
  void reply_consistency();

  void fenix_pre_recovery() { task = TaskT(); }
  int send(const void* b, int n, MPI_Datatype d, int t);
  int irecv(void* b, int n, MPI_Datatype d, int t, MPI_Request* r);
  fenix::tasks::Status wait(MPI_Request* r);

  std::string str() const;

 private:
  TaskT form_consistency();
  void ensure_consistency();

  const SendLog& log_send(const void* b, int n, MPI_Datatype d, int t);
  void replay_messages();

  void append_region(const Region& r);
  void erase_sends(const Region& r);
  void erase_regions(
    std::vector<Region>::iterator begin, std::vector<Region>::iterator end
  );

  // Check if two valid region vectors are consistent
  void check_consistent(std::vector<Region>& a, std::vector<Region>& b);
  void recover_invalid(std::vector<Region>& a, std::vector<Region>& b);

  std::set<SendLog, std::less<>> sends;
  // Last successful send as of last recovery, according to remote
  int already_sent = -1;

  // initialized by constructor and never resized, i.e. used as a dynamic array
  std::vector<Region> regions;

 public:
  Region& cur_region = regions.back();
  int& next_send = cur_region.send.next;
  int& next_recv = cur_region.recv.next;
};
} //namespace fenix::logging
#endif
