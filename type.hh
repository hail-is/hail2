#ifndef HAIL_TYPE_HH
#define HAIL_TYPE_HH

#include <ctype.h>

#include <string>
#include <vector>
#include <ostream>
#include <memory>
#include <regex>
#include <cassert>

#include <rapidjson/document.h>

#include "region.hh"
#include "util.hh"

class BaseType;
class Type;

extern std::ostream &operator<<(std::ostream &out, const BaseType &t);

extern std::shared_ptr<Type> parse_type(const char *s);

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
    BOOL,
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
  
  BaseType(Kind kind) : kind(kind) {}
  virtual ~BaseType();
  
  virtual std::ostream &put_to(std::ostream &out) const = 0;
};

class TMatrixTable : public BaseType {
public:
  // FIXME support compound keys
  const std::shared_ptr<Type> global_type;
  const std::shared_ptr<Type> col_key_type;
  const std::shared_ptr<Type> col_type;
  const std::shared_ptr<Type> row_key_type;
  const std::shared_ptr<Type> row_type;
  const std::shared_ptr<Type> entry_type;
  
  // FIXME name
  const std::shared_ptr<Type> row_impl_type;
  
  std::shared_ptr<Type> row_partition_key_type() const;
  
  // FIXME eliminate rapidjson dependence
  TMatrixTable(const rapidjson::Document &d);
  
  std::ostream &put_to(std::ostream &out) const;
};

class Type : public BaseType, public std::enable_shared_from_this<Type> {
public:
  const bool required;
  
  Type(Kind kind, bool required) : BaseType(kind), required(required) {}
  
  // FIXME devirtualize for non-codegen
  virtual uint64_t size() const = 0;
  virtual uint64_t alignment() const = 0;
  
  virtual std::shared_ptr<Type> fundamental_type() { return shared_from_this(); }
};

class TBoolean : public Type {
public:
  static constexpr Kind kindof = Kind::BOOL;
  
  TBoolean(bool required)
    : Type(Kind::BOOL, required) {}
  
  uint64_t size() const { return 1; }
  uint64_t alignment() const { return 1; }
  
  std::ostream &put_to(std::ostream &out) const;
};

class TInt32 : public Type {
public:
  static constexpr Kind kindof = Kind::INT32;
  
  TInt32(bool required)
    : Type(Kind::INT32, required) {}
  
  uint64_t size() const { return 4; }
  uint64_t alignment() const { return 4; }
  
  std::ostream &put_to(std::ostream &out) const;
};

class TInt64 : public Type {
public:
  static constexpr Kind kindof = Kind::INT64;
  
  TInt64(bool required)
    : Type(Kind::INT64, required) {}
  
  uint64_t size() const { return 8; }
  uint64_t alignment() const { return 8; }
  
  std::ostream &put_to(std::ostream &out) const;
};

class TFloat32 : public Type {
public:
  static constexpr Kind kindof = Kind::FLOAT32;
  
  TFloat32(bool required)
    : Type(Kind::FLOAT32, required) {}
  
  uint64_t size() const { return 4; }
  uint64_t alignment() const { return 4; }
  
  std::ostream &put_to(std::ostream &out) const;
};

class TFloat64 : public Type {
public:
  static constexpr Kind kindof = Kind::FLOAT64;
  
  TFloat64(bool required)
    : Type(Kind::FLOAT64, required) {}
  
  uint64_t size() const { return 8; }
  uint64_t alignment() const { return 8; }
  
  std::ostream &put_to(std::ostream &out) const;
};

class TString : public Type {
public:
  static constexpr Kind kindof = Kind::STRING;
  
  TString(bool required)
    : Type(Kind::STRING, required) {}
  
  uint64_t size() const { return 8; }
  uint64_t alignment() const { return 8; }
  
  uint64_t content_size(uint64_t length) const { return 4 + length; }
  uint64_t content_alignment(uint64_t length) const { return 4; }
  
  std::ostream &put_to(std::ostream &out) const;
};

class Field {
public:
  const std::string name;
  const std::shared_ptr<Type> type;
};

class TStruct : public Type {
public:
  static constexpr Kind kindof = Kind::STRUCT;
  
  const std::vector<Field> fields;
  
  // FIXME const, or private with accessors
  std::vector<uint64_t> field_offset;
  
  uint64_t n_nonrequired_fields;
  std::vector<uint64_t> field_missing_bit;
  
  uint64_t size_;
  uint64_t alignment_;
  
  TStruct(const std::vector<Field> &fields, bool required);
  
  // FIXME 1-time
  std::shared_ptr<Type> fundamental_type() override {
    std::vector<Field> new_fields;
    for (const auto &f : fields)
      new_fields.push_back(Field { f.name, f.type->fundamental_type() });
    return std::make_shared<TStruct>(new_fields, required);
  }
  
  uint64_t missing_bits_size() const { return (n_nonrequired_fields + 7) >> 3; }
  
  uint64_t size() const { return size_; }
  uint64_t alignment() const { return alignment_; }
  
  bool is_field_missing(const Region &region, uint64_t off, uint64_t i) {
    return !fields[i].type->required && region.load_bit(off, field_missing_bit[i]);
  }
  
  bool is_field_defined(const Region &region, uint64_t off, uint64_t i) {
    return !is_field_missing(region, off, i);
  }
  
  std::ostream &put_to(std::ostream &out) const;
};

class TArray : public Type {
public:
  static constexpr Kind kindof = Kind::ARRAY;
  
  const std::shared_ptr<Type> element_type;
  
  TArray(std::shared_ptr<Type> element_type, bool required)
    : Type(Kind::ARRAY, required),
      element_type(element_type) {}
  
  // FIXME 1-time
  std::shared_ptr<Type> fundamental_type() override {
    return std::make_shared<TArray>(element_type->fundamental_type(), required);
  }
  
  uint64_t missing_bits_size(uint64_t n) const {
    if (element_type->required)
      return 0;
    else
      return (n + 7) >> 3;
  }
  
  uint64_t element_size() const {
    return alignto(element_type->size(),
		   element_type->alignment());
  }
  
  uint64_t content_alignment() const {
    return std::max<uint64_t>(4, element_type->alignment());
  }
  uint64_t content_size(uint64_t n) const {
    return elements_offset(n) + n * element_size();
  }
  
  uint64_t elements_offset(uint64_t n) const {
    return alignto(4 + missing_bits_size(n),
		   element_type->alignment());
  }
  
  uint64_t size() const { return 8; }
  uint64_t alignment() const { return 8; }
  
  bool is_element_missing(const Region &region, uint64_t off, uint64_t i) {
    return !element_type->required && region.load_bit(off + 4, i);
  }
  
  bool is_element_defined(const Region &region, uint64_t off, uint64_t i) {
    return !is_element_missing(region, off, i);
  }
  
  std::ostream &put_to(std::ostream &out) const;
};

class TComplex : public Type {
public:
  const std::shared_ptr<Type> representation;
  
  TComplex(std::shared_ptr<Type> representation, Kind kind, bool required)
    : Type(kind, required), representation(representation) {}
  
  uint64_t size() const { return representation->size(); }
  uint64_t alignment() const { return representation->alignment(); }
  
  std::shared_ptr<Type> fundamental_type() override { return representation->fundamental_type(); }
};

class TSet : public TComplex {
public:
  static constexpr Kind kindof = Kind::SET;
  
  const std::shared_ptr<Type> element_type;
  
  TSet(std::shared_ptr<Type> element_type, bool required)
    : TComplex(std::make_shared<TArray>(element_type, required), Kind::SET, required),
      element_type(element_type) {}
  
  std::ostream &put_to(std::ostream &out) const;
};

class TCall : public TComplex {
public:
  static constexpr Kind kindof = Kind::CALL;
  
  TCall(bool required)
    : TComplex(std::make_shared<TInt32>(required), Kind::CALL, required) {}
  
  std::ostream &put_to(std::ostream &out) const;
};

class TLocus : public TComplex {
public:
  static constexpr Kind kindof = Kind::LOCUS;
  
  const std::string gr;
  
  TLocus(const std::string &gr, bool required)
    : TComplex(std::make_shared<TStruct>(
		 std::vector<Field> {
		   Field { "contig", std::make_shared<TString>(true) },
		     Field { "pos", std::make_shared<TInt32>(true) }
		 },
		 required),
	       Kind::LOCUS, required),
      gr(gr) {}
  
  std::ostream &put_to(std::ostream &out) const;
};

class TAltAllele : public TComplex {
public:
  static constexpr Kind kindof = Kind::ALTALLELE;
  
  TAltAllele(bool required)
    : TComplex(std::make_shared<TStruct>(
		 std::vector<Field> {
		   Field { "ref", std::make_shared<TString>(true) },
		     Field { "alt", std::make_shared<TString>(true) }
		 },
		 required),
	       Kind::ALTALLELE, required) {}
  
  std::ostream &put_to(std::ostream &out) const;
};

class TVariant : public TComplex {
public:
  static constexpr Kind kindof = Kind::VARIANT;
  
  const std::string gr;
  
  TVariant(const std::string &gr, bool required)
    : TComplex(std::make_shared<TStruct>(
		 std::vector<Field> {
		   Field { "contig", std::make_shared<TString>(true) },
		     Field { "pos", std::make_shared<TInt32>(true) },
		       Field { "ref", std::make_shared<TString>(true) },
			 Field { "altAlleles", std::make_shared<TArray>(
			     std::make_shared<TAltAllele>(true),
			     true) }
		 },
		 required),
	       Kind::VARIANT, required),
      gr(gr) {}
  
  std::ostream &put_to(std::ostream &out) const;
};

#endif // HAIL_TYPE_HH
