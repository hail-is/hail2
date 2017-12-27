#ifndef HAIL_MATRIXTABLE_HH
#define HAIL_MATRIXTABLE_HH

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <memory>

#include "type.hh"
#include "region.hh"
#include "inputbuffer.hh"

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
  std::shared_ptr<TMatrixTable> type;
  uint64_t n_partitions;
  
public:
  MatrixTable(const std::string &filename);
  
  MatrixTableIterator iterator() const;
  
  uint64_t count_rows() const;
};

#endif // HAIL_MATRIXTABLE_HH
