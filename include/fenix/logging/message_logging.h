#ifndef MESSAGE_LOGGING_H
#define MESSAGE_LOGGING_H
#include <istream>
#include <ostream>
#include <cassert>

#include <mpi.h>

#define CONSISTENCY_TAG 22234652
namespace fenix::logging {

namespace impl {

extern bool is_logging;
extern bool is_inline_recovering;

template <typename T>
class scoped_setting {
 public:
  scoped_setting(T& m_setting, T val) : setting(m_setting) { setting = val; }
  ~scoped_setting() { setting = old_val; }

 private:
  T& setting;
  const T old_val = setting;
};

class scoped_logging_setting {
 public:
  scoped_logging_setting(bool enabled) : setting(is_logging, enabled) {};
  scoped_setting<bool> setting;
};

class scoped_inline_setting {
 public:
  scoped_inline_setting(bool enabled)
    : log_setting(is_logging, enabled),
      inline_setting(is_inline_recovering, enabled) {};
  scoped_setting<bool> log_setting, inline_setting;
};

} //namespace impl

inline auto scoped_logging(bool state) {
  return impl::scoped_logging_setting(state);
}
inline auto scoped_inline_recovery(bool state) {
  return impl::scoped_inline_setting(state);
}

bool logging();
bool logging(bool set_state);

//Stack-based logging is recommended for consistency
//across faults.
template <typename F>
void with_logging(bool new_state, F&& func) {
  auto setting = scoped_logging(new_state);
  assert(logging() == new_state);
  func();
}

bool inline_recovery();
bool inline_recovery(bool set_state);

//This also sets with_logging, for ease of use.
template <typename F>
void with_inline_recovery(bool new_state, F&& func) {
  auto setting = scoped_inline_recovery(new_state);
  assert(logging() == new_state);
  assert(inline_recovery() == new_state);
  func();
}

//Local operation, but will throw away any existing logs
void init_message_logs(MPI_Comm& comm, int max_checkpoints = 2);

//Global calling semantics
void reset_message_consistency(int checkpoint_id = -1);

//Normally, we allow some asynchrony in coming to
//consistency. This makes sure all messages needing
//replay have been Isend'ed before exiting.
void ensure_message_consistency();

//Attempts progress with any outstanding inconsistent rank partners
void progress_message_consistency();

void begin_message_log_region(int region);

void store_message_logs(std::ostream& o);
void restore_message_logs(std::istream& i);
void restore_message_logs(std::istream& i, MPI_Comm& comm);

void store_message_logs(int group_id, int member_id);
void null_restore_message_logs(int group_id, int member_id);
void restore_message_logs(int group_id, int member_id);
void restore_message_logs(int group_id, int member_id, MPI_Comm& comm);
} //namespace fenix::logging
#endif
