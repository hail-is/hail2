
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <iterator>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

#include <fmt/format.h>

#include "gzstream.h"

#include "type.hh"
#include "region.hh"
#include "casting.hh"

void
pretty(std::ostream &out, Region &region, uint64_t off, const std::shared_ptr<Type> &t) {
  switch (t->kind) {
  case BaseType::Kind::BOOL:
    if (region.load_bool(off))
      out << "true";
    else
      out << "false";
    break;
  case BaseType::Kind::INT32:
    out << region.load_int(off);
    break;
  case BaseType::Kind::INT64:
    out << region.load_long(off);
    break;
  case BaseType::Kind::FLOAT32:
    out << region.load_float(off);
    break;
  case BaseType::Kind::FLOAT64:
    out << region.load_double(off);
    break;
  case BaseType::Kind::STRING:
    {
      uint64_t soff = region.load_offset(off);
      uint32_t n = region.load_int(soff);
      out << std::string((const char *)(region.mem + soff + 4), n);
    }
    break;
  case BaseType::Kind::STRUCT:
    {
      std::shared_ptr<TStruct> ts = cast<TStruct>(t);
      for (uint64_t i = 0; i < ts->fields.size(); ++i) {
	if (i > 0)
	  out << ", ";
	out << ts->fields[i].name << ": ";
	if (ts->is_field_defined(region, off, i))
	  pretty(out, region, off + ts->field_offset[i], ts->fields[i].type);
	else
	  out << ".";
      }
    }
    break;
  case BaseType::Kind::ARRAY:
    {
      std::shared_ptr<TArray> ta = cast<TArray>(t);
      uint64_t aoff = region.load_offset(off);
      uint32_t n = region.load_int(aoff);
      uint64_t elements_off = aoff + ta->elements_offset(n);
      uint64_t element_size = ta->element_size();
      out << "[" << n << "; ";
      for (uint64_t i = 0; i < n; ++i) {
	if (i > 0)
	  out << ", ";
	if (ta->is_element_defined(region, aoff, i))
	  pretty(out, region, elements_off + i*element_size, ta->element_type);
	else
	  out << ".";
      }
    }
    break;
  default: abort();
  }
}

void
decode_into(LZ4InputBuffer &in, Region &region, uint64_t off, const std::shared_ptr<Type> &t) {
  switch (t->kind) {
  case BaseType::Kind::BOOL:
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
      std::shared_ptr<TStruct> ts = cast<TStruct>(t);
      in.read_bytes(region, off, ts->missing_bits_size());
      for (uint64_t i = 0; i < ts->fields.size(); ++i)
	if (ts->is_field_defined(region, off, i))
	  decode_into(in, region, off + ts->field_offset[i], ts->fields[i].type);
    }
    break;
  case BaseType::Kind::ARRAY:
    {
      std::shared_ptr<TArray> ta = cast<TArray>(t);
      uint32_t n = in.read_int();
      uint64_t aoff = region.allocate(ta->content_alignment(),
				      ta->content_size(n));
      region.store_int(aoff, n);
      in.read_bytes(region, aoff + 4, ta->missing_bits_size(n));
      uint64_t elements_off = aoff + ta->elements_offset(n);
      uint64_t element_size = ta->element_size();
      for (uint64_t i = 0; i < n; ++i)
	if (ta->is_element_defined(region, aoff, i))
	  decode_into(in, region, elements_off + i*element_size, ta->element_type);
      region.store_offset(off, aoff);
    }
    break;
  default: abort();
  }
}

uint64_t
decode(LZ4InputBuffer &in, Region &region, const std::shared_ptr<Type> &t) {
  switch (t->kind) {
  case BaseType::Kind::BOOL:
    {
      uint64_t off = region.allocate(1, 1);
      region.store_bool(off, in.read_bool());
      return off;
    }
  case BaseType::Kind::INT32:
    {
      uint64_t off = region.allocate(4, 4);
      region.store_int(off, in.read_int());
      return off;
    }
  case BaseType::Kind::INT64:
    {
      uint64_t off = region.allocate(8, 8);
      region.store_long(off, in.read_long());
      return off;
    }
  case BaseType::Kind::FLOAT32:
    {
      uint64_t off = region.allocate(4, 4);
      region.store_float(off, in.read_float());
      return off;
    }
  case BaseType::Kind::FLOAT64:
    {
      uint64_t off = region.allocate(8, 8);
      region.store_double(off, in.read_double());
      return off;
    }
  case BaseType::Kind::STRING:
    {
      uint32_t n = in.read_int();
      uint64_t off = region.allocate(4, 4 + n);
      region.store_int(off, n);
      in.read_bytes(region, off + 4, n);
      return off;
    }
  case BaseType::Kind::STRUCT:
    {
      std::shared_ptr<TStruct> ts = cast<TStruct>(t);
      uint64_t off = region.allocate(t->alignment(), t->size());
      in.read_bytes(region, off, ts->missing_bits_size());
      for (uint64_t i = 0; i < ts->fields.size(); ++i)
	if (ts->is_field_defined(region, off, i))
	  decode_into(in, region, off + ts->field_offset[i], ts->fields[i].type);
      return off;
    }
  case BaseType::Kind::ARRAY:
    {
      std::shared_ptr<TArray> ta = cast<TArray>(t);
      uint32_t n = in.read_int();
      uint64_t off = region.allocate(ta->content_alignment(),
				     ta->content_size(n));
      region.store_int(off, n);
      in.read_bytes(region, off + 4, ta->missing_bits_size(n));
      uint64_t elements_off = off + ta->elements_offset(n);
      uint64_t element_size = ta->element_size();
      for (uint64_t i = 0; i < n; ++i) {
	if (ta->is_element_defined(region, off, i))
	  decode_into(in, region, elements_off + i*element_size, ta->element_type);
      }
      return off;
    }
  default: abort();
  }
}

int
main(int argc, char **argv) {
  // std::string vds = "/home/cotton/sample.vds";
  std::string vds = "/home/cotton/gnomad.vds";

  std::string metadata_filename = vds + "/metadata.json.gz";
  igzstream is(metadata_filename.c_str());
  std::string metadata;
  metadata.assign(std::istreambuf_iterator<char>(is),
		  std::istreambuf_iterator<char>());
  
  rapidjson::Document d;
  d.Parse(metadata.c_str());

  uint64_t n_partitions = d["n_partitions"].GetUint64();
  auto n_partitions_s = std::to_string(n_partitions);
  size_t n_digits = n_partitions_s.size();
  
  auto mtt = std::make_shared<TMatrixTable>(d);
  // std::cout << "mtt: " << *mtt;
  
  Region region;
  
  auto f = mtt->row_impl_type->fundamental_type();
  // std::cout << "f: " << *f << "\n";

#if 1
  uint64_t nrows = 0;
  uint64_t nbytes = 0;
  for (uint64_t i = 0; i < n_partitions; ++i) {
    auto is = std::to_string(i);
    std::string pad(n_digits - is.size(), '0');
    std::string part_filename = vds + "/parts/part-" + pad + is;
    
    // std::cout << part_filename << "...\n";
    
    int fd = open(part_filename.c_str(), O_RDONLY);
    LZ4InputBuffer in(fd);
    
    int8_t cont = in.read_byte();
    while (cont) {
      region.clear();
      // uint64_t off =
      decode(in, region, f);
      
      nbytes += region.end;
      
      // pretty(std::cout, region, off, f);
      // std::cout << "\n";
      
      ++nrows;
      cont = in.read_byte();
    }
  }
  
  std::cout << fmt::format("read {} rows, {}GiB decoded ", nrows, (double)nbytes / 1024 / 1024 / 1024) << "\n";
#endif
#if 0
  uint64_t nblocks = 0;
  uint64_t nbytes = 0;
  for (uint64_t i = 0; i < n_partitions; ++i) {
    auto is = std::to_string(i);
    std::string pad(n_digits - is.size(), '0');
    std::string part_filename = vds + "/parts/part-" + pad + is;
    
    // std::cout << part_filename << "...\n";
    
    int fd = open(part_filename.c_str(), O_RDONLY);
    LZ4InputBuffer in(fd);
    
    while (true) {
      bool b = in.maybe_read_block();
      if (!b)
	break;
      nblocks ++;
      nbytes += in.end;
      in.off = in.end;
    }
  }
  
  std::cout << fmt::format("read {} blocks, {}GiB decomp", nblocks, (double)nbytes / 1024 / 1024 / 1024) << "\n";
#endif

  
  return 0;
}
