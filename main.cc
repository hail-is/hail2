
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "region.hh"

class Decoder {
public:
  LZ4InputBuffer &in;
  Region &region;
  
  Decoder(LZ4InputBuffer &in_, Region &region_)
    : in(in_), region(region_) {}

#include "f"
};

int
main(int argc, char **argv)
{
  Region region;
  
  int fd = open(argv[1], O_RDONLY);
  LZ4InputBuffer in(fd);
  
  Decoder decoder(in, region);
  
  int nrows = 0;
  int8_t cont = in.read_byte();
  while (cont) {
    region.clear();
    
    decoder.read_rv();
    ++nrows;
    cont = in.read_byte();
  }
  
  printf("read %d rows\n", nrows);
}
