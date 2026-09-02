#ifndef FENIX_DATA_POLICY_LOCAL_HPP
#define FENIX_DATA_POLICY_LOCAL_HPP

#include <mpi.h>
#include <deque>
#include <algorithm>
#include <string>
#include <vector>
#include "fenix/data/group.hpp"
#include "fenix/data/member.hpp"
#include "fenix/data/subset.hpp"

namespace fenix::data::local {

struct LocalGroup : public DataGroup {
  LocalGroup(int id, MPI_Comm c, int ts, int depth)
    : DataGroup(id, c, ts, depth, FENIX_DATA_POLICY_LOCAL) {}

  mpixx::Group create_cohort() override {
    mpixx::Group comm_group = mpixx::Group::from_comm(comm);
    int rank;
    MPI_Comm_rank(comm, &rank);
    return mpixx::Group::incl(comm_group, {rank});
  }

  void get_redundant_policy(int* name, void* value) override {
    *name = FENIX_DATA_POLICY_LOCAL;
  }

  void member_restore_from_rank(
    int member_id, void* buffer, int max, int timestamp, int source_rank
  ) override {
    fenix_require(false, "unimplemented");
  }
};

} // namespace fenix::data::local

#endif //FENIX_DATA_POLICY_LOCAL_HPP
