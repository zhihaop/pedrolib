#include "pedrolib/timestamp.h"
#include <fmt/chrono.h>
#include <sys/time.h>

namespace pedrolib {

Timestamp Timestamp::Now() {
  struct timeval tv {};
  gettimeofday(&tv, nullptr);

  return Timestamp{tv.tv_sec * Duration::kMicroseconds + tv.tv_usec};
}
std::string Timestamp::String() const noexcept {
  time_t t = usecs / Duration::kMicroseconds;
  uint64_t us = usecs % Duration::kMicroseconds;
  return fmt::format("{:%Y-%m-%d-%H:%M:%S}:{:06}", fmt::localtime(t), us);
}
}  // namespace pedrolib