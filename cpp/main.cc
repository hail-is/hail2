
#include <iostream>
#include <memory>

#include <fmt/format.h>

#include "type.hh"
#include "context.hh"
#include "matrixtable.hh"

int
main(int argc, char **argv) {
  hail::Context c;
  
  std::string vds = "/home/cotton/sample.vds";
  //  std::string vds = "/home/cotton/gnomad.vds";
  
  auto mt = std::make_shared<hail::MatrixTable>(c, vds);
  std::cout << fmt::format("read {} rows ", mt->count_rows()) << "\n";
  
  return 0;
}
