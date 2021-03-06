
#include <unistd.h>

#include <cstdlib>

#define LZ4_DISABLE_DEPRECATE_WARNINGS
#include <lz4.h>

#include "inputbuffer.hh"

namespace hail {

LZ4InputBuffer::LZ4InputBuffer()
  : fd(-1),
    off(0),
    end(0) {
  buf = (char *)malloc(block_size);
  comp = (char *)malloc(4 + LZ4_compressBound(block_size));
}

LZ4InputBuffer::LZ4InputBuffer(int fd_)
  : fd(fd_),
    off(0),
    end(0) {
  buf = (char *)malloc(block_size);
  comp = (char *)malloc(4 + LZ4_compressBound(block_size));
}

LZ4InputBuffer &
LZ4InputBuffer::operator=(int fd_) {
  if (fd != -1)
    close(fd);
  fd = fd_;
  off = 0;
  end = 0;
  return *this;
}

LZ4InputBuffer::~LZ4InputBuffer() {
  close(fd);
  free(buf);
  free(comp);
}

void
LZ4InputBuffer::read_fully(void *dst0, size_t n) {
  assert(fd != -1);
  char *dst = (char *)dst0;
  while (n > 0) {
    ssize_t nread = read(fd, dst, n);
    assert(nread > 0);
    dst += nread;
    n -= nread;
  }
  assert(n == 0);
}
  
void
LZ4InputBuffer::read_block() {
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

} // namespace hail
