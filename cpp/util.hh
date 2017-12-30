#ifndef HAIL_UTIL_HH
#define HAIL_UTIL_HH
#pragma once

#include <cstdint>
#include <cassert>
#include <vector>

#define LIKELY(condition) __builtin_expect(static_cast<bool>(condition), 1)
#define UNLIKELY(condition) __builtin_expect(static_cast<bool>(condition), 0)
  
namespace hail {

inline uint64_t alignto(uint64_t p, uint64_t alignment) {
  assert(alignment > 0);
  assert((alignment & (alignment - 1)) == 0); // power of 2
  return (p + (alignment - 1)) & ~(alignment - 1);
}

template<typename T>
struct hash_points_to {
  std::size_t operator()(const T *const &p) const {
    std::hash<T> hasher;
    return hasher(*p);
  }
};

template<typename T> void
hash_combine(std::size_t &h, const T &x) {
  std::hash<T> hasher;
  h ^= hasher(x) + 0x9e3779b9 + (h << 6) + (h >> 2);
}

template<typename T>
struct equal_to_points_to {
  bool operator()(const T *const &lhs, const T *const &rhs) const {
    return *lhs == *rhs;
  }
};

} // namespace hail

namespace std {

template<typename T> struct hash<vector<T>> {
  std::size_t operator()(const vector<T> &v);
};

template<class T> std::size_t
hash<vector<T>>::operator()(const vector<T> &v) {
  using size_type = typename vector<T>::size_type;
  
  std::size_t h = std::hash<size_type>{}(v.size());
  for (size_type i = 0; i < v.size(); ++i)
    hash_combine(h, v[i]);
  return h;
}

}

#endif //  HAIL_UTIL_HH
