#ifndef PEDRODB_CONCURRENT_LATCH_H
#define PEDRODB_CONCURRENT_LATCH_H
#include <atomic>
#include <condition_variable>
#include <mutex>
#include "pedrolib/duration.h"
#include "pedrolib/logger/logger.h"

namespace pedrolib {

class Latch {
  std::atomic_size_t count_;
  std::mutex mu_;
  std::condition_variable zero_;

 public:
  explicit Latch(size_t count) : count_(count) {}

  [[nodiscard]] size_t Count() const noexcept {
    return count_.load(std::memory_order_acquire);
  }

  void Await() {
    std::unique_lock<std::mutex> lock(mu_);
    while (count_.load(std::memory_order_relaxed)) {
      zero_.wait(lock);
    }
  }

  bool Await(const Duration& d) {
    std::unique_lock<std::mutex> lock(mu_);
    auto st = std::chrono::steady_clock::now();
    auto et = st + std::chrono::microseconds(d.Microseconds());
    while (count_.load(std::memory_order_relaxed)) {
      auto now = std::chrono::steady_clock::now();
      if (now <= et) {
        break;
      }
      zero_.wait_until(lock, et);
    }
    return count_.load(std::memory_order_relaxed) == 0;
  }

  void CountDown() {
    size_t x;
    for (;;) {
      size_t n = count_.load(std::memory_order::memory_order_acquire);
      if (n == 0) {
        x = n;
        break;
      }
      x = n - 1;
      if (count_.compare_exchange_strong(n, x, std::memory_order_release)) {
        break;
      }
    }
    if (x == 0) {
      std::unique_lock<std::mutex> lock(mu_);
      zero_.notify_all();
    }
  }
};
}  // namespace pedrolib

#endif  // PEDRODB_CONCURRENT_LATCH_H
