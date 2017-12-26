#ifndef HAIL_REGION_HH
#define HAIL_REGION_HH

#include <cassert>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <unistd.h>

#define LZ4_DISABLE_DEPRECATE_WARNINGS
#include <lz4.h>

#include <cstdio>

#include "util.hh"

using offset_t = uint64_t;

class Region {
  // private:
public:
  char *mem;
  size_t capacity;
  size_t end;

public:
  Region()
    : mem((char *)malloc(128)), capacity(128), end(0) {}
  
  Region(size_t capacity_)
    : mem((char *)malloc(capacity_)), capacity(capacity_), end(0) {}

  ~Region() {
    free(mem);
  }
  
  void clear() { end = 0; }
  
  void grow(size_t required) {
    assert(capacity < required);
    
    size_t new_capacity = std::max((capacity * 3) >> 1, required);
    char *new_mem = (char *)malloc(new_capacity);
    assert(new_mem != nullptr);
    memcpy(new_mem, mem, end);
    free(mem);
    mem = new_mem;
    capacity = new_capacity;
  }
  
  offset_t allocate(offset_t alignment, offset_t n) {
    offset_t p = alignto(end, alignment);
    size_t new_end = p + n;
    if (capacity < new_end)
      grow(new_end);
    end = new_end;
    assert(end <= capacity);
    return p;
  }
  
  int32_t load_int(offset_t off) const {
    return *(int32_t *)(mem + off);
  }

  int64_t load_long(offset_t off) const {
    return *(int64_t *)(mem + off);
  }

  float load_float(offset_t off) const {
    return *(float *)(mem + off);
  }

  double load_double(offset_t off) const {
    return *(double *)(mem + off);
  }

  offset_t load_offset(offset_t off) const {
    return *(offset_t *)(mem + off);
  }
  
  int8_t load_byte(offset_t off) const {
    return *(int8_t *)(mem + off);
  }

  bool load_bool(offset_t off) const {
    return *(int8_t *)(mem + off) != 0;
  }
  
  bool load_bit(offset_t off, int i) const {
    int8_t b = *(mem + off + (i >> 3));
    return (b & (1 << (i & 7))) != 0;
  }
  
  void store_int(offset_t off, int32_t i) {
    *(int32_t *)(mem + off) = i;
  }

  void store_long(offset_t off, int64_t l) {
    *(int64_t *)(mem + off) = l;
  }

  void store_float(offset_t off, float f) {
    *(float *)(mem + off) = f;
  }

  void store_double(offset_t off, double d) {
    *(double *)(mem + off) = d;
  }

  void store_byte(offset_t off, int8_t l) {
    *(int8_t *)(mem + off) = l;
  }
  
  void store_bool(offset_t off, bool b) {
    *(int8_t *)(mem + off) = (int8_t)b;
  }
  
  void store_offset(offset_t off, offset_t o) const {
    *(offset_t *)(mem + off) = o;
  }
};

class RegionValue {
  // private:
public:
  const Region *region;
  offset_t offset;
  
public:
  RegionValue()
    : region(nullptr), offset(0)
  {}
  RegionValue(Region *region_, offset_t offset_)
    : region(region_), offset(offset_)
  {}
};

class LZ4InputBuffer {
  // private:
public:
  static const int block_size = 128 * 1024;
  
  int fd;
  char *buf;
  // FIXME don't store offsets store pointers
  size_t off;
  size_t end;
  
  char *comp;
  
  void read_fully(void *dst0, size_t n);
  void read_block();
  bool maybe_read_fully(void *dst0, size_t n);
  bool maybe_read_block();
  
  void ensure(size_t n) {
    if (off == end)
      read_block();
    assert(off + n <= end);
  }
  
public:
  LZ4InputBuffer(int fd_);
  ~LZ4InputBuffer();
  
  int8_t read_byte_() {
    assert(off < end);
    int8_t b = *(int8_t *)(buf + off);
    off += 1;
    return b;
  }
  
  int8_t read_byte() {
    ensure(1);
    int8_t b = *(int8_t *)(buf + off);
    off += 1;
    return b;
  }
  
  bool read_bool() { return read_byte() != 0; }
  
  float read_float() {
    ensure(4);
    float f = *(float *)(buf + off);
    off += 4;
    return f;
  }
  
  double read_double() {
    ensure(8);
    double d = *(double *)(buf + off);
    off += 8;
    return d;
  }
  
#define LIKELY(condition) __builtin_expect(static_cast<bool>(condition), 1)
#define UNLIKELY(condition) __builtin_expect(static_cast<bool>(condition), 0)
  
  int32_t read_int() {
    ensure(1);
    
    int8_t b = read_byte_();
    int32_t x = b & 0x7f;
    int shift = 7;
    while ((b & 0x80) != 0) {
      b = read_byte_();
      x |= ((b & 0x7f) << shift);
      shift += 7;
    }
    return x;
  }

  int64_t read_long() {
    ensure(1);
    
    int8_t b = read_byte_();
    int64_t x = b & 0x7f;
    int shift = 7;
    while ((b & 0x80) != 0) {
      b = read_byte_();
      x |= ((b & 0x7f) << shift);
      shift += 7;
    }
    return x;
  }
  
  void read_bytes(Region &region, offset_t roff, size_t n) {
    while (n > 0) {
      if (end == off)
        read_block();
      int p = std::min(end - off, n);
      assert(p > 0);
      // FIXME call on region
      memcpy(region.mem + roff, buf + off, p);
      roff += p;
      n -= p;
      off += p;
    }
  }
};

#endif // HAIL_REGION_HH
