
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cassert>
#include <iostream>
#include <memory>

#include "type.hh"
#include "region.hh"

class Decoder {
  LZ4InputBuffer &in;
  Region &region;
  
public:
  Decoder(LZ4InputBuffer &in, Region &region)
    : in(in), region(region) {}
  
#include "f"
};

int
main(int argc, char **argv)
{
  Region region;
  
  const char *ts = "Struct { a: Array[Int32] }";
  for (TokenIterator i(ts); !i.at_end(); ++i) {
    std::cout << *i << "\n";
  }
  
  auto tt = parse_type(ts);
  std::cout << "tt: " << *tt << "\n";
  
  std::shared_ptr<Type> t = std::make_shared<TStruct>(
    std::vector<Field> {
      {"pk", std::make_shared<TInt32>(false)},
      {"v", std::make_shared<TInt32>(false)},
      {"va", std::make_shared<TStruct>(std::vector<Field> {}, false)},
      {"gs", std::make_shared<TArray>(std::make_shared<TFloat64>(false), true)}
    }, true);
  std::cout << *t << "\n";
  
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
