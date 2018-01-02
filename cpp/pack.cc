 
#include <ctime>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

#include <emmintrin.h>
#include <tmmintrin.h>
#include <immintrin.h>

void
print_bytes(void *p, size_t n) {
  uint8_t *q = (uint8_t *)p;
  for (size_t i = 0; i < n; ++i) {
    if (i > 0)
      printf(" ");
    printf("%02x", q[i]);
  }
  printf("\n");
}

void
init(uint8_t *p, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    int x = random();
    if ((x % 5) == 0) { // 80% 0
      p[i] = random();
    } else
      p[i] = 0;
  }
}

size_t
pack_slow(uint8_t *in, size_t n, uint8_t *out) {
  assert((n & 0xf) == 0);
  
  // k: output index
  size_t k = 0;
  // i: block
  for (size_t i = 0; i < (n >> 4); ++i) {
    size_t mi = k;
    k += 2;
    
    uint16_t m = 0;
    // j: byte in block
    for (int j = 0; j < 16; ++j) {
      uint8_t c = in[i * 16 + j];
      if (c != 0) {
	m |= (1 << j);
	out[k] = c;
	k += 1;
      }
    }
    *(uint16_t *)(out + mi) = m;
  }
  return k;
}

static uint64_t control_table[256];

void compute_control_table() {
  for (int m = 0; m < 256; ++m) {
    uint64_t x = 0;
    int k = 0;
    for (int j = 0; j < 8; ++j) {
      if ((m & (1 << j)) != 0) {
	x |= ((uint64_t)k << (j * 8));
	++k;
      } else
	x |= ((uint64_t)0x80 << (j * 8));
    }
    control_table[m] = x;
  }
}

void unpack_fast(uint8_t *in, size_t unpacked_size, uint8_t *out) {
  assert((unpacked_size & 0xf) == 0);
  for (size_t i = 0; i < (unpacked_size >> 4); ++i) {
    uint16_t m = *(uint16_t *)in;
    in += 2;
    int n0 = _mm_popcnt_u32(m & 0xff);
    int n = _mm_popcnt_u32(m);
    
    __m128i x = _mm_loadu_si128((const __m128i *)in);
    __m128i ctrl = _mm_setr_epi64(_m_from_int64(control_table[m & 0xff]),
				  _mm_add_pi8(_m_from_int64(control_table[m >> 8]),
					      _mm_set1_pi8(n0)));
    __m128i u = _mm_shuffle_epi8(x, ctrl);
    _mm_storeu_si128((__m128i *)out, u);
    out += 16;
    in += n;
  }
}

int
main() {
  compute_control_table();
  
  size_t n = 1024 * 1024 * 1024; // 1GiB
  printf("n: %ld\n", n);
  
  uint8_t *in = (uint8_t *)malloc(n);
  
  size_t packed_capacity = ((n + 7) / 8 * 9) + 8;
  printf("packed_capacity: %ld\n", packed_capacity);
  uint8_t *packed = (uint8_t *)malloc(packed_capacity);
  
  uint8_t *out = (uint8_t *)malloc(n);
  
  printf("initializing...\n");
  init(in, n);
  
  printf("packing...\n");
  size_t packed_size = pack_slow(in, n, packed);
  printf("packed_size: %ld\n", packed_size);
  
  printf("timing...\n");
  
  // discard the first few
  for (int i = 0; i < 3; ++i) {
    unpack_fast(packed, n,  out);
  }
  
  struct timespec before, after;
  clock_gettime(CLOCK_REALTIME, &before);

  int iterations = 10;
  for (int i = 0; i < iterations; ++i) {
    unpack_fast(packed, n,  out);
  }
  clock_gettime(CLOCK_REALTIME, &after);
  
  int64_t elapsed = ((int64_t)((after.tv_sec - before.tv_sec) * 1000000000)
		     - (int64_t)before.tv_nsec
		     + (int64_t)after.tv_nsec) / iterations;
  
  printf("elapsed: %fms\n", (double)elapsed / 1e6);
  
  // print_bytes(in, n);
  // print_bytes(packed, packed_capacity);
  // print_bytes(out, n);
  
  assert(memcmp(in, out, n) == 0);
  
  return 0;
}
