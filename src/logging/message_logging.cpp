#include <sstream>
#include "fenix.hpp"
#include "fenix_util.hpp"
#include "fenix/logging/message_logging.h"
#include "fenix/logging/serialize.h"
#include "fenix/logging/util.h"
#include "fenix/logging/comm_log.h"

namespace fenix::logging {
namespace impl {
bool is_logging = false;
bool is_inline_recovering = false;
}

bool logging() { return impl::is_logging; }
bool logging(bool set_state) {
  bool old = impl::is_logging;
  impl::is_logging = set_state;
  return old;
}

bool inline_recovery() {
  return impl::is_logging && impl::is_inline_recovering;
}
bool inline_recovery(bool set_state) {
  bool old = impl::is_inline_recovering;
  impl::is_inline_recovering = set_state;
  return old;
}

void progress_message_consistency() {
  try {
    comm_log.value().progress();
  } catch (fenix::CommException& e) {
    if (!inline_recovery()) throw;
  }
}

void ensure_message_consistency() {
  assert(!logging());
  do {
    progress_message_consistency();
  } while (!comm_log.value().tasks.empty());
}

void begin_message_log_region(int region) {
  comm_log.value().begin_region(region);
}

void init_message_logs(MPI_Comm& comm, int max_checkpoints) {
  comm_log.emplace(comm, max_checkpoints);

  fenix::callback_register(
    [](MPI_Comm, int) { comm_log.value().fenix_pre_recovery(); },
    fenix::PRE_RECOVERY
  );
}

void reset_message_consistency(int checkpoint_id) {
  comm_log.value().reset_consistency(checkpoint_id);
}

void serialize_message_logs(std::ostream& o) {
  if (!comm_log) return;
  comm_log.value().serialize(o);
}
void stage_message_logs(int group_id, int member_id) {
  if (!comm_log) return;

  using namespace fenix;
  using namespace fenix::data;
  if (!member_created(group_id, member_id)) {
    assert(group_created(group_id));
    int ret =
      member_create(group_id, member_id, nullptr, FENIX_RESIZEABLE, MPI_BYTE);
    assert(ret == FENIX_SUCCESS);
  }

  std::stringstream o;
  comm_log.value().serialize(o);

  size_t size = o.tellp();
  void* ptr = (void*)o.view().data();
  MLOG("%s storing logs of size %d\n", comm_log.value().str().c_str(), size);

  int flag = 0;
  Fenix_Data_member_attr_set(
    group_id, member_id, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, ptr, &flag
  );
  int ret = member_stage(group_id, member_id, DataSubset(size - 1));
  assert(ret == FENIX_SUCCESS);
}

void deserialize_message_logs(std::istream& i, MPI_Comm& comm) {
  bool existing = !!comm_log;
  comm_log.emplace(comm, i);
  if (!existing) {
    fenix::callback_register(
      [](MPI_Comm, int) { comm_log.value().fenix_pre_recovery(); },
      fenix::PRE_RECOVERY
    );
  }
}
void deserialize_message_logs(std::istream& i) {
  deserialize_message_logs(i, comm_log.value().comm);
}

fenix::DataSubset impl_null_restore_message_logs(int group_id, int member_id) {
  fenix::DataSubset subset;
  int ret = fenix::data::member_restore(
    group_id, member_id, nullptr, 0, FENIX_DATA_SNAPSHOT_LATEST, subset
  );
  if (ret != FENIX_SUCCESS)
    fatal_print(
      "Rank %d error restoring message logs (%d)",
      util::comm_rank(MPI_COMM_WORLD), ret
    );

  int length = subset.max_count();
  assert(length > 0 && subset.includes_all(length - 1));
  return subset;
}
void null_restore_message_logs(int group_id, int member_id) {
  impl_null_restore_message_logs(group_id, member_id);
}

void restore_message_logs(int group_id, int member_id, MPI_Comm& comm) {
  auto subset = impl_null_restore_message_logs(group_id, member_id);
  int length = subset.max_count();

  // Initialize a long enough buffer string
  std::string buf(static_cast<std::string::size_type>(length), ' ');
  int ret = fenix::data::member_lrestore(
    group_id, member_id, &buf[0], length, FENIX_DATA_SNAPSHOT_LATEST, subset
  );
  if (ret != FENIX_SUCCESS)
    fatal_print(
      "Rank %d error lrestoring message logs (%d)", util::comm_rank(comm), ret
    );

  std::istringstream i(std::move(buf));
  assert(i.view().size() == length);
  deserialize_message_logs(i, comm);
}
void restore_message_logs(int group_id, int member_id) {
  restore_message_logs(group_id, member_id, comm_log.value().comm);
}
} //namespace fenix::logging
