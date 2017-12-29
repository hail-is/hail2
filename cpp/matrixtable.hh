#ifndef HAIL_MATRIXTABLE_HH
#define HAIL_MATRIXTABLE_HH
#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <memory>

#include "region.hh"
#include "inputbuffer.hh"

namespace hail {

class TMatrixTable;
class MatrixTable;

class MatrixTableIterator {
  std::shared_ptr<const MatrixTable> mt;
  
  Region region;
  
  uint64_t part;
  LZ4InputBuffer in;
  
  void start_part();
  void advance();
  
public:
  MatrixTableIterator(const std::shared_ptr<const MatrixTable> &mt);
  
  bool has_next() const;
  
  TypedRegionValue next();
};

class MatrixTable : public std::enable_shared_from_this<MatrixTable> {
public:
  std::string filename;
  const TMatrixTable *type;
  uint64_t n_partitions;
  
public:
  MatrixTable(Context &c, const std::string &filename);
  
  // FIXME unique_ptr, but had trouble with unique_ptr in Cython
  std::shared_ptr<MatrixTableIterator> iterator() const;
  
  uint64_t count_rows() const;
};

#endif // HAIL_MATRIXTABLE_HH

} // namespace hail
  
