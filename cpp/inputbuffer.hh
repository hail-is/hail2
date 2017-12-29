#ifndef HAIL_INPUTBUFFER_HH
#define HAIL_INPUTBUFFER_HH
#pragma once

#include "util.hh"
#include "region.hh"

namespace hail {

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
  
  // stats
  uint64_t ninput, noutput;
  
  void read_fully(void *dst0, size_t n);
  void read_block();
  
  void ensure(size_t n) {
    if (UNLIKELY(off == end))
      read_block();
    assert(off + n <= end);
  }
  
public:
  LZ4InputBuffer();
  LZ4InputBuffer(int fd_);
  ~LZ4InputBuffer();
  
  LZ4InputBuffer &operator=(int fd_);
  
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
  
  int32_t read_int() {
    ensure(4);
    int32_t i = *(int32_t *)(buf + off);
    off += 4;
    return i;
  }

  int64_t read_long() {
    ensure(8);
    int64_t l = *(int64_t *)(buf + off);
    off += 8;
    return l;
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

} // namespace hail

#endif // HAIL_INPUTBUFFER_HH
