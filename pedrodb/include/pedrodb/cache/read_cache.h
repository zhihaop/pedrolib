#ifndef PEDRODB_CACHE_READ_CACHE_H
#define PEDRODB_CACHE_READ_CACHE_H

#include "pedrodb/defines.h"
#include <pedrolib/collection/lru_unordered_map.h>

namespace pedrodb {
template <typename K, typename V, typename Hash = std::hash<K>>
using lru_hash_map = pedrolib::lru::unordered_map<K, V, Hash>;

class ReadCache {
  using Hash = ValueLocationHash;

  lru_hash_map<ValueLocation, std::string, Hash> read_cache_;
  std::unordered_map<uint32_t, std::string> active_cache_;
  uint32_t active_id_{};

  size_t size_{};
  const size_t capacity_;

  uint64_t hits_{1};
  uint64_t total_{1};

public:
  explicit ReadCache(size_t capacity) : capacity_(capacity) {}

  double HitRatio() const noexcept {
    return static_cast<double>(hits_) / total_;
  }

  void UpdateCache(ValueLocation location, std::string_view value) {
    if (location.id == active_id_) {
      active_cache_[location.offset].assign(value.begin(), value.end());
    } else {
      if (value.size() > capacity_) {
        return;
      }
      while (size_ + value.size() > capacity_) {
        size_ -= read_cache_.evict().size();
      }
      size_ += value.size();
      read_cache_.update(location, std::string{value});
    }
  }

  void Remove(ValueLocation location) {
    if (location.id == active_id_) {
      active_cache_.erase(location.offset);
    } else {
      if (read_cache_.contains(location)) {
        size_ -= read_cache_.erase(location).size();
      }
    }
  }

  bool Read(ValueLocation location, std::string *value) {
    ++total_;
    if (location.id == active_id_) {
      auto iter = active_cache_.find(location.offset);
      if (iter == active_cache_.end()) {
        return false;
      }
      *value = iter->second;
      ++hits_;
      return true;
    }

    if (!read_cache_.contains(location)) {
      return false;
    }

    *value = read_cache_[location];
    ++hits_;
    return true;
  }

  void UpdateActiveID(uint32_t active_id) {
    for (auto &[k, v] : active_cache_) {
      ValueLocation location{
          .id = active_id,
          .offset = k,
      };
      if (v.size() > capacity_) {
        continue;
      }
      while (size_ + v.size() > capacity_) {
        size_ -= read_cache_.evict().size();
      }
      size_ += v.size();
      read_cache_.update(location, std::move(v));
    }
    active_id_ = active_id;
    active_cache_.clear();
  }
};
} // namespace pedrodb

#endif // PEDRODB_CACHE_READ_CACHE_H