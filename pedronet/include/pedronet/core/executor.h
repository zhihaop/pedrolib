#ifndef PEDRONET_CORE_EXECUTOR_H
#define PEDRONET_CORE_EXECUTOR_H

#include "pedronet/core/duration.h"
#include "pedronet/core/noncopyable.h"
#include "pedronet/core/nonmovable.h"
#include <functional>

namespace pedronet::core {
using CallBack = std::function<void()>;
struct Executor : core::noncopyable, core::nonmovable {
  virtual void Schedule(CallBack cb) = 0;
  virtual uint64_t ScheduleAfter(CallBack cb, core::Duration delay) = 0;
  virtual uint64_t ScheduleEvery(CallBack cb, core::Duration delay,
                                 core::Duration interval) = 0;
  virtual void ScheduleCancel(uint64_t) = 0;
};

} // namespace pedronet::core

#endif // PEDRONET_CORE_EXECUTOR_H