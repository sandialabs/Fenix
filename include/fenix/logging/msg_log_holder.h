#ifndef MSG_LOG_HOLDER_H
#define MSG_LOG_HOLDER_H

#include "fenix/logging/msg_log.h"

namespace fenix::logging {
using MsgTypes = std::tuple<class SendLog>;

template <typename LogType>
constexpr int MsgTypeId = static_cast<int>(util::TupleIndex<LogType, LogTypes>);

template <typename LogT = void>
class LogHolder {
 public:
  std::string str() const;

  std::unique_ptr<MsgLog> log;
  int type = -1;
}
}

#endif //MSG_LOG_HOLDER_H
