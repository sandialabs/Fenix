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

  MPI_Group create_cohort() override {
    MPI_Group comm_group;
    MPI_Comm_group(comm, &comm_group);
    int rank;
    MPI_Comm_rank(comm, &rank);
    MPI_Group cohort_group;
    MPI_Group_incl(comm_group, 1, &rank, &cohort_group);
    MPI_Group_free(&comm_group);
    return cohort_group;
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
