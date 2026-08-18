#include "fenix_data_component.hpp"
#include "fenix_data_group.hpp"
#include "fenix_data_policy_in_memory_raid.hpp"
#include "fenix_data_policy_local.hpp"
#include "fenix_ext.hpp"
#include <mpi.h>

namespace fenix::data {

bool GroupIdComparator::operator()(
  const std::shared_ptr<fenix_group_t>& a,
  const std::shared_ptr<fenix_group_t>& b
) const {
  return a->groupid < b->groupid;
}

bool GroupIdComparator::operator()(
  const std::shared_ptr<fenix_group_t>& a, int id
) const {
  return a->groupid < id;
}

bool GroupIdComparator::operator()(
  int id, const std::shared_ptr<fenix_group_t>& a
) const {
  return id < a->groupid;
}

DataComponent::~DataComponent() = default;

fenix_group_t* DataComponent::search_group(int id) {
  auto iter = groups.find(id);
  if (iter == groups.end()) return nullptr;
  return iter->get();
}

fenix_group_t* DataComponent::find_group(int id, std::source_location loc) {
  auto group = this->search_group(id);
  if (!group) FENIX_THROW_FROM(FENIX_ERROR_INVALID_GROUPID, loc);
  return group;
}

void DataComponent::group_create(
  int groupid, MPI_Comm comm, int timestart, int depth, int policy_name,
  void* policy_value
) {
  auto group = this->search_group(groupid);

  if (!group) {
    // Create new group based on policy
    fenix_group_t* new_group = nullptr;
    switch (policy_name) {
    case FENIX_DATA_POLICY_IN_MEMORY_RAID:
      new_group =
        new imr::Group(groupid, comm, timestart, depth, (int*)policy_value);
      break;
    case FENIX_DATA_POLICY_LOCAL:
      new_group = new local::LocalGroup(groupid, comm, timestart, depth);
      break;
    default:
      FENIX_THROW(FENIX_ERROR_INVALID_POLICY_NAME);
    }

    // Add to component
    groups.insert(std::shared_ptr<fenix_group_t>(new_group));
    group_order.push_back(groupid);
  } else {
    // Already created. Renew the MPI communicator
    group->comm = comm;
    MPI_Comm_rank(comm, &(group->current_rank));

    // Reinit group metadata as needed w/ new communicator
    group->reinit();
  }
}

void DataComponent::remove_group(int id) {
  auto iter = groups.find(id);
  if (iter == groups.end()) FENIX_THROW(FENIX_ERROR_INVALID_GROUPID);

  groups.erase(iter);

  for (int i = 0; i < group_order.size(); i++) {
    if (group_order[i] == id) {
      group_order.erase(group_order.begin() + i);
      break;
    }
  }
}

fenix_member_entry_t* DataComponent::search_member(int groupid, int memberid) {
  auto g = this->search_group(groupid);
  return g ? g->search_member(memberid) : nullptr;
}

fenix_member_entry_t* DataComponent::find_member(
  int groupid, int memberid, std::source_location loc
) {
  return this->find_group(groupid, loc)->find_member(memberid, loc);
}

} // namespace fenix::data
