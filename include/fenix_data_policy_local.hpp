#ifndef FENIX_DATA_POLICY_LOCAL_HPP
#define FENIX_DATA_POLICY_LOCAL_HPP

#include <mpi.h>
#include <deque>
#include <algorithm>
#include <string>
#include <vector>
#include "fenix_data_group.hpp"
#include "fenix_data_member.hpp"
#include "fenix_data_subset.hpp"

namespace fenix::data::local {

struct LocalGroup : public DataGroup {
  LocalGroup(int id, MPI_Comm c, int ts, int depth)
    : DataGroup(id, c, ts, depth, FENIX_DATA_POLICY_LOCAL) {}

  void get_redundant_policy(int* name, void* value) override {
    *name = FENIX_DATA_POLICY_LOCAL;
  }

  // Local policy has no reliability, there is no repair or reinit to do
  void reinit() override {};
  void member_repair(int member_id) override {}

  void member_restore_from_rank(
    int member_id, void* buffer, int max, int timestamp, int source_rank
  ) override {
    fenix_require(false, "unimplemented");
  }
};

} // namespace fenix::data::local

#endif //FENIX_DATA_POLICY_LOCAL_HPP
