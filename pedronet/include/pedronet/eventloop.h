#ifndef PEDRONET_EVENTLOOP_H
#define PEDRONET_EVENTLOOP_H

#include "pedronet/callbacks.h"
#include "pedronet/channel/channel.h"
#include "pedronet/channel/event_channel.h"
#include "pedronet/channel/timer_channel.h"
#include "pedronet/core/duration.h"
#include "pedronet/core/executor.h"
#include "pedronet/core/noncopyable.h"
#include "pedronet/core/nonmovable.h"
#include "pedronet/core/thread.h"
#include "pedronet/event.h"
#include "pedronet/selector/selector.h"
#include "timer_queue.h"

namespace pedronet {

class EventLoop : public core::Executor {
  inline const static core::Duration kSelectTimeout{std::chrono::seconds(10)};

  std::unique_ptr<Selector> selector_;
  EventChannel event_channel_;
  TimerChannel timer_channel_;
  TimerQueue timer_queue_;

  std::mutex mu_;
  std::vector<Callback> pending_tasks_;
  std::vector<Callback> running_tasks_;

  std::atomic_int32_t state_{1};
  std::unordered_map<Channel *, Callback> channels_;

  int32_t state() const noexcept {
    return state_.load(std::memory_order_acquire);
  }

public:
  explicit EventLoop(std::unique_ptr<Selector> selector);

  Selector *GetSelector() noexcept { return selector_.get(); }

  void Deregister(Channel *channel);

  void Register(Channel *channel, Callback register_callback,
                Callback deregister_callback);

  bool CheckUnderLoop() const noexcept {
    return core::Thread::Current().CheckUnderLoop(this);
  }

  void AssertUnderLoop() const;

  void Schedule(Callback cb) override;

  uint64_t ScheduleAfter(core::Duration delay, Callback cb) override {
    return timer_queue_.ScheduleAfter(delay, std::move(cb));
  }

  uint64_t ScheduleEvery(core::Duration delay, core::Duration interval,
                         Callback cb) override {
    return timer_queue_.ScheduleEvery(delay, interval, std::move(cb));
  }

  void ScheduleCancel(uint64_t id) override { timer_queue_.Cancel(id); }

  template <typename Runnable> void Run(Runnable &&runnable) {
    if (CheckUnderLoop()) {
      runnable();
      return;
    }
    Schedule(std::forward<Runnable>(runnable));
  }

  bool Closed() const noexcept { return state() == 0; }

  void Close();

  void Loop();
};

} // namespace pedronet

#endif // PEDRONET_EVENTLOOP_H