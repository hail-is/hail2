#ifndef HAIL_UTIL_HH
#define HAIL_UTIL_HH
#pragma once

#include <cstdint>
#include <cassert>

#define LIKELY(condition) __builtin_expect(static_cast<bool>(condition), 1)
#define UNLIKELY(condition) __builtin_expect(static_cast<bool>(condition), 0)
  
namespace hail {

inline uint64_t alignto(uint64_t p, uint64_t alignment) {
  assert(alignment > 0);
  assert((alignment & (alignment - 1)) == 0); // power of 2
  return (p + (alignment - 1)) & ~(alignment - 1);
}

} // namespace hail

#endif //  HAIL_UTIL_HH
