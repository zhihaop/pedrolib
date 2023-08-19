#ifndef PEDROLIB_CONCURRENT_SPINLOCK_H
#define PEDROLIB_CONCURRENT_SPINLOCK_H

#include <pthread.h>
#include <atomic>
#include <thread>

namespace pedrolib {
class SpinLock {
  pthread_spinlock_t actual_{};

 public:
  SpinLock() { pthread_spin_init(&actual_, PTHREAD_PROCESS_PRIVATE); }

  ~SpinLock() { pthread_spin_destroy(&actual_); }

  void lock() noexcept { pthread_spin_lock(&actual_); }

  bool try_lock() noexcept { return pthread_spin_trylock(&actual_); }

  void unlock() noexcept { pthread_spin_unlock(&actual_); }
};
}  // namespace pedrolib
#endif  // PEDROLIB_CONCURRENT_SPINLOCK_H
