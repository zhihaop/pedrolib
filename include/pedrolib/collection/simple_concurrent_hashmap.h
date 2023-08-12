#ifndef PEDROLIB_COLLECTION_SIMPLE_CONCURRENT_HASHMAP_H
#define PEDROLIB_COLLECTION_SIMPLE_CONCURRENT_HASHMAP_H

#include <mutex>
#include <thread>
#include <unordered_map>
#include "static_vector.h"

namespace pedrolib {

template <typename K, typename V, typename Hash = std::hash<K>,
          typename Pred = std::equal_to<K>,
          typename Alloc = std::allocator<std::pair<const K, V>>>
class SimpleConcurrentHashMap {

  struct HashMap {
    std::mutex mu_;
    std::unordered_map<K, V, Hash, Pred, Alloc> table_;

    [[nodiscard]] size_t size() const noexcept {
      std::lock_guard guard{mu_};
      return table_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
      std::lock_guard guard{mu_};
      return table_.empty();
    }

    bool at(const K& key, V& value) {
      std::lock_guard guard{mu_};
      auto it = table_.find(key);
      if (it == table_.end()) {
        return false;
      }
      value = it->second;
      return true;
    }

    template <class Value>
    bool insert(const K& key, Value&& value) {
      std::lock_guard guard{mu_};
      auto it = table_.find(key);
      if (it == table_.end()) {
        table_[key] = std::forward<Value>(value);
        return true;
      }
      return false;
    }
  };

  StaticVector<HashMap> tables_;
  Hash hash_;

 public:
  explicit SimpleConcurrentHashMap(size_t segments) : tables_(segments) {}

  SimpleConcurrentHashMap()
      : SimpleConcurrentHashMap(std::thread::hardware_concurrency()) {}

  size_t segments(const K& key) const noexcept {
    return hash_(key) % tables_.size();
  }

  [[nodiscard]] size_t size() const noexcept {
    size_t sz = 0;
    for (const auto& table : tables_) {
      sz += table.size();
    }
    return sz;
  }

  bool at(const K& key, V& value) {
    size_t b = segments(key);
    return tables_[b].at(key, value);
  }
  
  template <class Value>
  bool insert(const K& key, Value&& value) {
    size_t b = segments(key);
    return tables_[b].insert(key, std::forward<Value>(value));
  }
};
}  // namespace pedrolib

#endif  // PEDROLIB_COLLECTION_SIMPLE_CONCURRENT_HASHMAP_H
