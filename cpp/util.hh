#ifndef HAIL_UTIL_HH
#define HAIL_UTIL_HH

namespace hail {

inline uint64_t alignto(uint64_t p, uint64_t alignment) {
  assert(alignment > 0);
  assert((alignment & (alignment - 1)) == 0); // power of 2
  return (p + (alignment - 1)) & ~(alignment - 1);
}

} // namespace hail

#endif //  HAIL_UTIL_HH
