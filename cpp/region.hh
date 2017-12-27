#ifndef HAIL_REGION_HH
#define HAIL_REGION_HH

#include <cassert>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <unistd.h>

#include <cstdio>

#include "casting.hh"
#include "type.hh"
#include "util.hh"

using offset_t = uint64_t;

class Region {
  // private:
public:
  char *mem;
  size_t capacity;
  size_t end;

public:
  Region()
    : mem((char *)malloc(128)), capacity(128), end(0) {}
  
  Region(size_t capacity_)
    : mem((char *)malloc(capacity_)), capacity(capacity_), end(0) {}

  ~Region() {
    free(mem);
  }
  
  void clear() { end = 0; }
  
  void grow(size_t required) {
    assert(capacity < required);
    
    size_t new_capacity = std::max((capacity * 3) >> 1, required);
    char *new_mem = (char *)malloc(new_capacity);
    assert(new_mem != nullptr);
    memcpy(new_mem, mem, end);
    free(mem);
    mem = new_mem;
    capacity = new_capacity;
  }
  
  offset_t allocate(offset_t alignment, offset_t n) {
    offset_t p = alignto(end, alignment);
    size_t new_end = p + n;
    if (capacity < new_end)
      grow(new_end);
    end = new_end;
    assert(end <= capacity);
    return p;
  }
  
  int32_t load_int(offset_t off) const {
    return *(int32_t *)(mem + off);
  }

  int64_t load_long(offset_t off) const {
    return *(int64_t *)(mem + off);
  }

  float load_float(offset_t off) const {
    return *(float *)(mem + off);
  }

  double load_double(offset_t off) const {
    return *(double *)(mem + off);
  }

  offset_t load_offset(offset_t off) const {
    return *(offset_t *)(mem + off);
  }
  
  int8_t load_byte(offset_t off) const {
    return *(int8_t *)(mem + off);
  }

  bool load_bool(offset_t off) const {
    return *(int8_t *)(mem + off) != 0;
  }
  
  bool load_bit(offset_t off, int i) const {
    int8_t b = *(mem + off + (i >> 3));
    return (b & (1 << (i & 7))) != 0;
  }
  
  void store_int(offset_t off, int32_t i) {
    *(int32_t *)(mem + off) = i;
  }

  void store_long(offset_t off, int64_t l) {
    *(int64_t *)(mem + off) = l;
  }

  void store_float(offset_t off, float f) {
    *(float *)(mem + off) = f;
  }

  void store_double(offset_t off, double d) {
    *(double *)(mem + off) = d;
  }

  void store_byte(offset_t off, int8_t l) {
    *(int8_t *)(mem + off) = l;
  }
  
  void store_bool(offset_t off, bool b) {
    *(int8_t *)(mem + off) = (int8_t)b;
  }
  
  void store_offset(offset_t off, offset_t o) const {
    *(offset_t *)(mem + off) = o;
  }
  
  bool is_field_missing(const std::shared_ptr<TStruct> &ts, uint64_t off, uint64_t i) const {
    return !ts->fields[i].type->required && load_bit(off, ts->field_missing_bit[i]);
  }
  
  bool is_field_defined(const std::shared_ptr<TStruct> &ts, uint64_t off, uint64_t i) const {
    return !is_field_missing(ts, off, i);
  }
  
  bool is_element_missing(const std::shared_ptr<TArray> &ta, uint64_t off, uint64_t i) const {
    return !ta->element_type->required && load_bit(off + 4, i);
  }
  
  bool is_element_defined(const std::shared_ptr<TArray> &ta, uint64_t off, uint64_t i) const {
    return !is_element_missing(ta, off, i);
  }
};

class RegionValue {
  // private:
public:
  const Region &region;
  offset_t offset;
  
public:
  RegionValue(const Region &region_, offset_t offset_)
    : region(region_), offset(offset_)
  {}
};

class TypedRegionValue {
public:
  const Region &region;
  offset_t offset;
  std::shared_ptr<Type> type;
  
  void pretty_1(std::ostream &out, uint64_t off, const std::shared_ptr<Type> &t) const;
  
public:
  TypedRegionValue(const Region &region, offset_t offset, const std::shared_ptr<Type> &type)
    : region(region), offset(offset), type(type)
  {}
  
  bool is_field_missing(uint64_t i) {
    return region.is_field_missing(cast<TStruct>(type), offset, i);
  }
  
  bool is_field_defined(uint64_t i) {
    return region.is_field_defined(cast<TStruct>(type), offset, i);
  }
  
  bool load_field_bool(uint64_t i) {
    auto ts = cast<TStruct>(type);
    assert(isa<TBoolean>(ts->fields[i].type));
    return region.load_bool(offset + ts->field_offset[i]);
  }
  
  int32_t load_field_int(uint64_t i) {
    auto ts = cast<TStruct>(type);
    assert(isa<TInt32>(ts->fields[i].type));
    return region.load_int(offset + ts->field_offset[i]);
  }

  int64_t load_field_long(uint64_t i) {
    auto ts = cast<TStruct>(type);
    assert(isa<TInt64>(ts->fields[i].type));
    return region.load_long(offset + ts->field_offset[i]);
  }
  
  float load_field_float(uint64_t i) {
    auto ts = cast<TStruct>(type);
    assert(isa<TFloat32>(ts->fields[i].type));
    return region.load_float(offset + ts->field_offset[i]);
  }
  
  double load_field_double(uint64_t i) {
    auto ts = cast<TStruct>(type);
    assert(isa<TFloat64>(ts->fields[i].type));
    return region.load_double(offset + ts->field_offset[i]);
  }
  
  uint64_t load_field_offset(uint64_t i) {
    auto ts = cast<TStruct>(type);
    // FIXME predicate for value as offset/pointer
    assert(isa<TArray>(ts->fields[i].type)
	   || isa<TString>(ts->fields[i].type));
    return region.load_offset(offset + ts->field_offset[i]);
  }
  
  bool is_element_missing(uint64_t i) {
    return region.is_element_missing(cast<TArray>(type), offset, i);
  }
  
  bool is_element_defined(uint64_t i) {
    return region.is_element_defined(cast<TArray>(type), offset, i);
  }
  
  bool load_element_bool(uint64_t i) {
    auto ta = cast<TArray>(type);
    assert(isa<TBoolean>(ta->element_type));
    uint64_t n = region.load_int(offset);
    return region.load_bool(offset + ta->element_offset(n, i));
  }
  
  int32_t load_element_int(uint64_t i) {
    auto ta = cast<TArray>(type);
    assert(isa<TInt32>(ta->element_type));
    uint64_t n = region.load_int(offset);
    return region.load_int(offset + ta->element_offset(n, i));
  }

  int64_t load_element_long(uint64_t i) {
    auto ta = cast<TArray>(type);
    assert(isa<TInt64>(ta->element_type));
    uint64_t n = region.load_int(offset);
    return region.load_long(offset + ta->element_offset(n, i));
  }
  
  float load_element_float(uint64_t i) {
    auto ta = cast<TArray>(type);
    assert(isa<TFloat32>(ta->element_type));
    uint64_t n = region.load_int(offset);
    return region.load_float(offset + ta->element_offset(n, i));
  }
  
  double load_element_double(uint64_t i) {
    auto ta = cast<TArray>(type);
    assert(isa<TFloat64>(ta->element_type));
    uint64_t n = region.load_int(offset);
    return region.load_double(offset + ta->element_offset(n, i));
  }
  
  uint64_t load_element_offset(uint64_t i) {
    auto ta = cast<TArray>(type);
    assert(isa<TArray>(ta->element_type)
	   || isa<TString>(ta->element_type));
    uint64_t n = region.load_int(offset);
    return region.load_offset(offset + ta->element_offset(n, i));
  }
  
  void pretty(std::ostream &out) const {
    pretty_1(out, offset, type->fundamental_type());
  }
};

#endif // HAIL_REGION_HH
