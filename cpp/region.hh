#ifndef HAIL_REGION_HH
#define HAIL_REGION_HH
#pragma once

#include <cassert>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <unistd.h>

#include <cstdio>

#include "casting.hh"
#include "type.hh"
#include "util.hh"

namespace hail {

class TypedRegionValue;

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
    if (UNLIKELY(capacity < new_end))
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
  
  bool is_field_missing(const TStruct *ts, uint64_t off, uint64_t i) const {
    return !ts->fields[i].type->required && load_bit(off, ts->field_missing_bit[i]);
  }
  
  bool is_field_defined(const TStruct *ts, uint64_t off, uint64_t i) const {
    return !is_field_missing(ts, off, i);
  }
  
  bool is_element_missing(const TArray *ta, uint64_t off, uint64_t i) const {
    return !ta->element_type->required && load_bit(off + 4, i);
  }
  
  bool is_element_defined(const TArray *ta, uint64_t off, uint64_t i) const {
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

extern std::ostream &operator<<(std::ostream &out, const TypedRegionValue &t);

class TypedRegionValue {
  const Region *region;
  offset_t offset;
public:
  const Type *type;
  
  std::ostream &put_to(std::ostream &out, uint64_t off, const Type *t) const;
  
public:
  TypedRegionValue() = default;
  TypedRegionValue(const Region *region, offset_t offset, const Type *type)
    : region(region), offset(offset), type(type)
  {}
  
  bool load_bool() const {
    assert(isa<TBoolean>(type->fundamental_type));
    return region->load_bool(offset);
  }
  
  int32_t load_int() const {
    assert(isa<TInt32>(type->fundamental_type));
    return region->load_int(offset);
  }
  
  int64_t load_long() const {
    assert(isa<TInt64>(type->fundamental_type));
    return region->load_long(offset);
  }
  
  float load_float() const {
    assert(isa<TFloat32>(type->fundamental_type));
    return region->load_float(offset);
  }
  
  double load_double() const {
    assert(isa<TFloat64>(type->fundamental_type));
    return region->load_double(offset);
  }
  
  std::string load_string() const {
    assert(isa<TString>(type->fundamental_type));
    uint64_t soff = region->load_offset(offset);
    uint32_t n = region->load_int(soff);
    return std::string((const char *)(region->mem + soff + 4), n);
  }
  
  bool is_field_missing(uint64_t i) {
    return region->is_field_missing(cast<TStruct>(type->fundamental_type), offset, i);
  }
  
  bool is_field_defined(uint64_t i) {
    return region->is_field_defined(cast<TStruct>(type->fundamental_type), offset, i);
  }

  TypedRegionValue load_field(uint64_t i) {
    auto ts = cast<TStruct>(type->fundamental_type);
    return TypedRegionValue(region, offset + ts->field_offset[i], ts->fields[i].type);
  }
  
  bool is_element_missing(uint64_t i) {
    return region->is_element_missing(cast<TArray>(type->fundamental_type), offset, i);
  }
  
  bool is_element_defined(uint64_t i) {
    return region->is_element_defined(cast<TArray>(type->fundamental_type), offset, i);
  }
  
  uint64_t array_size() {
    assert(isa<TArray>(type->fundamental_type));
    uint64_t aoff = region->load_offset(offset);
    return region->load_int(aoff);
  }
  
  TypedRegionValue load_element(uint64_t i) {
    auto ta = cast<TArray>(type->fundamental_type);
    uint64_t aoff = region->load_offset(offset);
    uint64_t n = region->load_int(aoff);
    return TypedRegionValue(region, aoff + ta->element_offset(n, i), ta->element_type);
  }
  
  std::ostream &put_to(std::ostream &out) const;
  
  std::string to_string() const;
};

} // namespace hail

#endif // HAIL_REGION_HH
