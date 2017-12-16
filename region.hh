
#include <cassert>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <unistd.h>

#define LZ4_DISABLE_DEPRECATE_WARNINGS
#include <lz4.h>

#include <cstdio>

using offset_t = uint64_t;

inline offset_t alignto(offset_t p, offset_t alignment) {
  assert(alignment > 0);
  assert((alignment & (alignment - 1)) == 0); // power of 2
  return (p + (alignment - 1)) & ~(alignment - 1);
}

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
  
  void align(offset_t alignment) {
    end = alignto(end, alignment);
  }
  
  void grow() {
    size_t new_capacity = std::max((capacity * 3) >> 1, end);
    char *new_mem = (char *)malloc(new_capacity);
    memcpy(new_mem, mem, end);
    free(mem);
    mem = new_mem;
    capacity = new_capacity;
  }
  
  offset_t allocate(offset_t n) {
    offset_t p = end;
    end += n;
    if (capacity < end)
      grow();
    return p;
  }
  
  int32_t load_int(offset_t off) const {
    return *(int32_t *)(mem + off);
  }

  int64_t load_long(offset_t off) const {
    return *(int64_t *)(mem + off);
  }

  double load_float(offset_t off) const {
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

  void store_float(offset_t off, double f) {
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
  
  void read_fully(void *dst0, size_t n) {
    char *dst = (char *)dst0;
    while (n > 0) {
      ssize_t nread = read(fd, dst, n);
      assert(nread > 0);
      dst += nread;
      n -= nread;
    }
    assert(n == 0);
  }
  
  void read_block() {
    assert(off == end);
    
    // read the header
    int32_t comp_len;
    read_fully(&comp_len, 4);
    
    read_fully(comp, 4 + comp_len);
    int decomp_len = *(int32_t *)comp;

#ifndef NDEBUG
    int comp_len2 =
#endif
      LZ4_decompress_fast(comp + 4, buf, decomp_len);
    assert(comp_len2 == comp_len);
    
    off = 0;
    end = decomp_len;
  }
  
  void ensure(size_t n) {
    if (off == end)
      read_block();
    assert(off + n <= end);
  }
  
public:
  LZ4InputBuffer(int fd_)
    : fd(fd_),
      off(0),
      end(0) {
    buf = (char *)malloc(block_size);
    comp = (char *)malloc(4 + LZ4_compressBound(block_size));
  }
  
  ~LZ4InputBuffer() {
    close(fd);
    free(buf);
    free(comp);
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
    int8_t f = *(float *)(buf + off);
    off += 4;
    return f;
  }
  
  double read_double() {
    ensure(8);
    int8_t d = *(double *)(buf + off);
    off += 8;
    return d;
  }
  
#define LIKELY(condition) __builtin_expect(static_cast<bool>(condition), 1)
#define UNLIKELY(condition) __builtin_expect(static_cast<bool>(condition), 0)
  
  int32_t read_int() {
    int8_t b = read_byte();
    int32_t x = b & 0x7f;
    int shift = 7;
    while ((b & 0x80) != 0) {
      b = read_byte();
      x |= ((b & 0x7f) << shift);
      shift += 7;
    }
    return x;
  }

  int32_t read_int2() {
    int8_t b0 = read_byte();
    int32_t x = b0 & 0x7f;
    if (UNLIKELY((b0 & 0x80) != 0)) {
      int8_t b1 = read_byte();
      x |= ((b1 & 0x7f) << 7);
      if (UNLIKELY((b1 & 0x80) != 0)) {
	int8_t b2 = read_byte();
	x |= ((b2 & 0x7f) << 14);
	if (UNLIKELY((b2 & 0x80) != 0)) {
	  int8_t b3 = read_byte();
	  x |= ((b3 & 0x7f) << 21);
	  if (UNLIKELY((b3 & 0x80) != 0)) {
	    int8_t b4 = read_byte();
	    x |= ((b4 & 0x7f) << 28);
	  }
	}
      }
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
