#ifndef FENIX_LOGGING_COLLECTIVE_LOG_HOLDER_H
#define FENIX_LOGGING_COLLECTIVE_LOG_HOLDER_H

#include <type_traits>
#include <variant>
#include <memory>

#include "fenix/logging/op_log.h"
#include "fenix/logging/serialize.h"
#include "fenix/logging/ops/barrier_log.h"
#include "fenix/logging/ops/bcast_log.h"
#include "fenix/logging/ops/reduce_log.h"
#include "fenix/logging/ops/allreduce_log.h"

namespace fenix::logging {

using CollectiveLogVariant =
  std::variant<BarrierLog*, BcastLog*, ReduceLog*, AllreduceLog*>;

// Type-erasing helper that can be serialized/deserialized directly.
// Treat as a unique_ptr
class CollectiveLogHolder {
 public:
  CollectiveLogHolder() = default;
  CollectiveLogHolder(CollectiveLogHolder&& o) { *this = std::move(o); }
  CollectiveLogHolder& operator=(CollectiveLogHolder&& o) {
    log = std::move(o.log);
    variant = std::move(o.variant);
    return *this;
  }

  // Create based on underlying log type
  template <typename LogT, typename... Args>
  static CollectiveLogHolder create(Args... args) {
    auto l = std::make_unique<LogT>(args...);
    return CollectiveLogHolder(std::move(l), static_cast<LogT*>(l.get()));
  }

  CollectiveLogHolder(std::istream& i) {
    read_log(
      i, serialize::read<size_t>(i),
      std::make_index_sequence<std::variant_size_v<CollectiveLogVariant>>{}
    );
  }

  void serialize(std::ostream& o) const {
    fenix_assert(log);
    serialize::write<size_t>(o, variant.index());
    serialize::write(o, *log);
  }

  operator bool() const { return !!log; }
  CollectiveLog* operator->() { return log.get(); }
  const CollectiveLog* operator->() const { return log.get(); }

  // Support placing in a sorted set
  auto operator<=>(const CollectiveLogHolder& o) const {
    fenix_assert(log);
    return log->idx() <=> o->idx();
  }
  auto operator<=>(const int& i) const {
    fenix_assert(log);
    return log->idx() <=> i;
  }
  auto operator==(const int& i) const {
    fenix_assert(log);
    return log->idx() == i;
  }

  // Helper to try reading log as each possible type
  template <size_t... I>
  void read_log(std::istream& input, size_t type, std::index_sequence<I...>) {
    ((read_log<I>(input, type)), ...);
    fenix_assert(log);
  }

  // Helper to try reading log as type with index I, no-op if type mismatch
  template <size_t I>
  void read_log(std::istream& input, size_t type) {
    if (type != I) return;
    using LogT = std::remove_pointer_t<
      std::variant_alternative_t<I, CollectiveLogVariant>>;
    log = std::make_unique<LogT>(input);
    variant.emplace<I>(static_cast<LogT*>(log.get()));
  }

  std::unique_ptr<CollectiveLog> log;
  CollectiveLogVariant variant;

 private:
  CollectiveLogHolder(
    std::unique_ptr<CollectiveLog>&& m_log, CollectiveLogVariant&& m_variant
  )
    : log(std::move(m_log)), variant(std::move(m_variant)) {}
};

} //namespace fenix::logging

#endif
