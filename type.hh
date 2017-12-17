
#include <string>
#include <vector>
#include <ostream>
#include <memory>

class BaseType;

extern std::ostream &operator <<(std::ostream &out, const BaseType &t);

class BaseType {
public:
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
    STRUCT,
    ARRAY,
    // compound types
    CALL,
    LOCUS,
    ALT_ALLELE,
    VARIANT,
    INTERVAL,
  };
  
  Kind kind;
  
  BaseType(Kind kind) : kind(kind) {}
  virtual ~BaseType();
  
  virtual std::ostream &put_to(std::ostream &out) const = 0;
};

class Type : public BaseType {
public:
  const bool required;
  
  Type(Kind kind, bool required) : BaseType(kind), required(required) {}
};

class TBoolean : public Type {
public:
  static constexpr Kind kindof = Kind::INT32;
  
  TBoolean(bool required)
    : Type(BaseType::Kind::INT32, required) {}
  
  virtual std::ostream &put_to(std::ostream &out) const;
};

class TInt32 : public Type {
public:
  static constexpr Kind kindof = Kind::INT32;
  
  TInt32(bool required)
    : Type(BaseType::Kind::INT32, required) {}
  
  virtual std::ostream &put_to(std::ostream &out) const;
};

class TFloat64 : public Type {
public:
  static constexpr Kind kindof = Kind::FLOAT64;
  
  TFloat64(bool required)
    : Type(BaseType::Kind::INT32, required) {}
  
  virtual std::ostream &put_to(std::ostream &out) const;
};

class Field {
public:
  const std::string name;
  const std::shared_ptr<Type> type;
};

class TStruct : public Type {
public:
  const std::vector<Field> fields;
  
  TStruct(const std::vector<Field> &fields, bool required)
    : Type(BaseType::Kind::STRUCT, required),
      fields(fields) {}
  
  virtual std::ostream &put_to(std::ostream &out) const;
};

class TArray : public Type {
public:
  static constexpr Kind kindof = Kind::ARRAY;
  
  const std::shared_ptr<Type> element_type;
  
  TArray(std::shared_ptr<Type> element_type, bool required)
    : Type(BaseType::Kind::ARRAY, required),
      element_type(element_type) {}
  
  virtual std::ostream &put_to(std::ostream &out) const;
};
