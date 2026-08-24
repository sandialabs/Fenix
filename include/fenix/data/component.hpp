#ifndef FENIX_DATA_COMPONENT_HPP
#define FENIX_DATA_COMPONENT_HPP

#include <memory>
#include <set>
#include <vector>
#include <source_location>
#include "fenix/data/group.hpp"

namespace fenix::data {

class DataComponent {
 public:
  DataComponent() = default;
  ~DataComponent();

  std::set<std::shared_ptr<DataGroup>, DataGroupIdComparator> groups;
  std::vector<int> group_order;

  // Search for group by id, returning null if not found
  DataGroup* search_group(int id);
  DataMember* search_member(int groupid, int memberid);

  // As search functions, but throw if not found
  DataGroup* find_group(
    int id, std::source_location loc = std::source_location::current()
  );
  DataMember* find_member(
    int groupid, int memberid,
    std::source_location loc = std::source_location::current()
  );

  // Create and add a group to the component
  void group_create(
    int groupid, MPI_Comm comm, int timestart, int depth, int policy_name,
    void* policy_value
  );

  // Remove a group by id
  void remove_group(int id);

  // Get number of groups
  size_t count() const { return groups.size(); }
};

} // namespace fenix::data

#endif // FENIX_DATA_COMPONENT_HPP
