#ifndef HAIL_TYPE_HH
#define HAIL_TYPE_HH
#pragma once

#include <ctype.h>

#include <string>
#include <vector>
#include <ostream>
#include <memory>
#include <cassert>

#include "util.hh"

namespace hail {

class BaseType;
class Type;
class Context;

extern std::ostream &operator<<(std::ostream &out, const BaseType &t);

extern Type *parse_type(const char *s);

class BaseType {
public:
  using Base = BaseType;
  
  enum class Kind
  {
    // relational types
    MATRIXTABLE,
    TABLE,
    // distributed linear algebra type
    MATRIX,
    // scalar types
    // fundamental types
    BOOLEAN,
    INT32,
    INT64,
    FLOAT32,
    FLOAT64,
    STRING,
    STRUCT,
    ARRAY,
    SET,
    // compound types
    CALL,
    LOCUS,
    ALTALLELE,
    VARIANT,
    INTERVAL,
  };
  
  Kind kind;
  
protected:
  BaseType(Kind kind) : kind(kind) {}
  
public:
  virtual ~BaseType();
  
  std::string to_string() const;
  
  virtual std::ostream &put_to(std::ostream &out) const = 0;
};

class TMatrixTable : public BaseType {
  friend class Context;
  
  TMatrixTable(Context &c,
	       const Type *global_type,
	       const Type *col_key_type,
	       const Type *col_type,
	       const Type *row_key_type,
	       const Type *row_type,
	       const Type *entry_type);
  
public:
  // FIXME support compound keys
  const Type *global_type;
  const Type *col_key_type;
  const Type *col_type;
  const Type *row_key_type;
  const Type *row_type;
  const Type *entry_type;
  
  // FIXME name
  const Type *row_impl_type;
  
  std::ostream &put_to(std::ostream &out) const;
};

class Type : public BaseType {
  friend class Context;

protected:
  Type(Kind kind, bool required);
  Type(Kind kind, bool required, uint64_t alignment, uint64_t size);
  Type(Kind kind, bool required, uint64_t alignment, uint64_t size, const Type *fundamental_type);
  
public:
  bool required;
  
  uint64_t alignment;
  uint64_t size;
  
  const Type *fundamental_type;
  
  bool is_fundamental() const { return fundamental_type == this; }
};

class TBoolean : public Type {
  friend class Context;
  
  TBoolean(bool required);
  
public:
  static constexpr Kind kindof = Kind::BOOLEAN;
  
  std::ostream &put_to(std::ostream &out) const;
};

class TInt32 : public Type {
  friend class Context;
  
  TInt32(bool required);
  
public:
  static constexpr Kind kindof = Kind::INT32;
  
  std::ostream &put_to(std::ostream &out) const;
};

class TInt64 : public Type {
  friend class Context;
  
  TInt64(bool required);
  
public:
  static constexpr Kind kindof = Kind::INT64;
  
  std::ostream &put_to(std::ostream &out) const;
};

class TFloat32 : public Type {
  friend class Context;
  
  TFloat32(bool required);
  
public:
  static constexpr Kind kindof = Kind::FLOAT32;
  
  std::ostream &put_to(std::ostream &out) const;
};

class TFloat64 : public Type {
  friend class Context;
  
  TFloat64(bool required);
  
public:
  static constexpr Kind kindof = Kind::FLOAT64;
  
  std::ostream &put_to(std::ostream &out) const;
};

class TString : public Type {
  friend class Context;
  
  TString(bool required);
  
public:
  static constexpr Kind kindof = Kind::STRING;
  
  uint64_t content_size(uint64_t length) const { return 4 + length; }
  uint64_t content_alignment(uint64_t length) const { return 4; }
  
  std::ostream &put_to(std::ostream &out) const;
};

class Field {
public:
  std::string name;
  const Type *type;
};

class TStruct : public Type {
  friend class Context;
  
  TStruct(Context &c, const std::vector<Field> &fields, bool required);
  
public:
  static constexpr Kind kindof = Kind::STRUCT;
  
  std::vector<Field> fields;
  
  std::vector<uint64_t> field_offset;
  
  uint64_t n_nonrequired_fields;
  std::vector<uint64_t> field_missing_bit;
  
  uint64_t missing_bits_size() const { return (n_nonrequired_fields + 7) >> 3; }
  
  std::ostream &put_to(std::ostream &out) const;
};

class TArray : public Type {
  friend class Context;
  
  TArray(Context &c, const Type *element_type, bool required);
  
public:
  static constexpr Kind kindof = Kind::ARRAY;
  
  const Type *element_type;
  
  uint64_t missing_bits_size(uint64_t n) const {
    if (element_type->required)
      return 0;
    else
      return (n + 7) >> 3;
  }
  
  uint64_t element_size() const {
    return alignto(element_type->size,
		   element_type->alignment);
  }
  
  uint64_t content_alignment() const {
    return std::max<uint64_t>(4, element_type->alignment);
  }
  uint64_t content_size(uint64_t n) const {
    return elements_offset(n) + n * element_size();
  }
  
  uint64_t elements_offset(uint64_t n) const {
    return alignto(4 + missing_bits_size(n),
		   element_type->alignment);
  }
  
  uint64_t element_offset(uint64_t n, uint64_t i) const {
    assert(i < n);
    return elements_offset(n) + i * element_size();
  }
  
  std::ostream &put_to(std::ostream &out) const;
};

class TComplex : public Type {
public:
  const Type *representation;
  
protected:
  TComplex(const Type *representation, Kind kind, bool required);
};

class TSet : public TComplex {
  friend class Context;
  
  TSet(Context &c, const Type *element_type, bool required);
  
public:
  static constexpr Kind kindof = Kind::SET;
  
  const Type *element_type;
  
  std::ostream &put_to(std::ostream &out) const;
};

class TCall : public TComplex {
  friend class Context;
  
  TCall(Context &c, bool required);
  
public:
  static constexpr Kind kindof = Kind::CALL;
  
  std::ostream &put_to(std::ostream &out) const;
};

class TLocus : public TComplex {
  friend class Context;
  
  TLocus(Context &c, const std::string &gr, bool required);
  
public:
  static constexpr Kind kindof = Kind::LOCUS;
  
  std::string gr;
  
  std::ostream &put_to(std::ostream &out) const;
};

class TAltAllele : public TComplex {
  friend class Context;
  
  TAltAllele(Context &c, bool required);
  
public:
  static constexpr Kind kindof = Kind::ALTALLELE;
  
  std::ostream &put_to(std::ostream &out) const;
};

class TVariant : public TComplex {
  friend class Context;
  
  TVariant(Context &c, const std::string &gr, bool required);
  
public:
  static constexpr Kind kindof = Kind::VARIANT;
  
  const std::string gr;
  
  std::ostream &put_to(std::ostream &out) const;
};

} // namespace hail

#endif // HAIL_TYPE_HH
