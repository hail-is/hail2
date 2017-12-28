
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

#include "gzstream.h"

#include "context.hh"
#include "matrixtable.hh"

namespace hail {

// FIXME move to ... region?
void
decode(LZ4InputBuffer &in, Region &region, uint64_t off, const Type *t) {
  switch (t->kind) {
  case BaseType::Kind::BOOLEAN:
    region.store_bool(off, in.read_bool());
    break;
  case BaseType::Kind::INT32:
    region.store_int(off, in.read_int());
    break;
  case BaseType::Kind::INT64:
    region.store_long(off, in.read_long());
    break;
  case BaseType::Kind::FLOAT32:
    region.store_float(off, in.read_float());
    break;
  case BaseType::Kind::FLOAT64:
    region.store_double(off, in.read_double());
    break;
  case BaseType::Kind::STRING:
    {
      uint32_t n = in.read_int();
      uint64_t soff = region.allocate(4, 4 + n);
      region.store_int(soff, n);
      in.read_bytes(region, soff + 4, n);
      region.store_offset(off, soff);
    }
    break;
  case BaseType::Kind::STRUCT:
    {
      const TStruct *ts = cast<TStruct>(t);
      in.read_bytes(region, off, ts->missing_bits_size());
      for (uint64_t i = 0; i < ts->fields.size(); ++i)
	if (region.is_field_defined(ts, off, i))
	  decode(in, region, off + ts->field_offset[i], ts->fields[i].type);
    }
    break;
  case BaseType::Kind::ARRAY:
    {
      const TArray *ta = cast<TArray>(t);
      uint32_t n = in.read_int();
      uint64_t aoff = region.allocate(ta->content_alignment(),
				      ta->content_size(n));
      region.store_int(aoff, n);
      in.read_bytes(region, aoff + 4, ta->missing_bits_size(n));
      uint64_t elements_off = aoff + ta->elements_offset(n);
      uint64_t element_size = ta->element_size();
      for (uint64_t i = 0; i < n; ++i)
	if (region.is_element_defined(ta, aoff, i))
	  decode(in, region, elements_off + i*element_size, ta->element_type);
      region.store_offset(off, aoff);
    }
    break;
  default: abort();
  }
}

void
MatrixTableIterator::start_part() {
  int n_digits = std::to_string(mt->n_partitions).size();
  
  auto part_s = std::to_string(part);
  std::string pad(n_digits - part_s.size(), '0');
  std::string part_filename = mt->filename + "/parts/part-" + pad + part_s;
  
  int fd = open(part_filename.c_str(), O_RDONLY);
  assert(fd != -1);
  in = fd;
}

void
MatrixTableIterator::advance() {
  bool cont = in.read_byte();
  while (!cont && part < mt->n_partitions) {
    ++part;
    if (part < mt->n_partitions) {
      start_part();
      cont = in.read_byte();
    }
  }
}

MatrixTableIterator::MatrixTableIterator(const std::shared_ptr<const MatrixTable> &mt)
  : mt(mt),
    part(0) {
  start_part();
  advance();
}

bool
MatrixTableIterator::has_next() const {
  return part < mt->n_partitions;
}

TypedRegionValue
MatrixTableIterator::next() {
  const auto &row_impl = mt->type->row_impl_type;
  
  region.clear();
  uint64_t offset = region.allocate(row_impl->alignment,
				    row_impl->size);
  decode(in, region, offset, row_impl->fundamental_type);
  
  advance();
  
  return TypedRegionValue(region, offset, row_impl);
}

MatrixTable::MatrixTable(Context &c, const std::string &filename)
  : filename(filename) {
  std::string metadata_filename = filename + "/metadata.json.gz";
  igzstream is(metadata_filename.c_str());
  std::string metadata;
  metadata.assign(std::istreambuf_iterator<char>(is),
		  std::istreambuf_iterator<char>());
  
  rapidjson::Document d;
  d.Parse(metadata.c_str());
  
  type = c.matrix_table_type(d);
  n_partitions = d["n_partitions"].GetUint64();
}

std::shared_ptr<MatrixTableIterator>
MatrixTable::iterator() const {
  return std::make_unique<MatrixTableIterator>(shared_from_this());
}

uint64_t
MatrixTable::count_rows() const {
  auto i = iterator();
  uint64_t nrows = 0;
  while (i->has_next()) {
    i->next();
    ++nrows;
  }
  return nrows;
}

} // namespace hail
