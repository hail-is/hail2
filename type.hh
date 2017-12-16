
#include <string>
#include <vector>

class Type {
protected:
  enum class Kind
  {
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

private:
  const Kind m_kind;
  const bool m_required;
  
public:
  Type(Kind kind, bool required) : m_kind(kind), m_required(required) {}
  
  bool required() const { return m_required; }
  
  Kind kind() const { return m_kind; }
};

class TInt32 : Type {
public:
  TInt32(bool required)
    : Type(Type::Kind::INT32, required) {}
};

class TFloat64 : Type {
public:
  TInt32(bool required)
    : Type(Type::Kind::INT32, required) {}
};

class Field {
public:
  const std::string m_name;
  const Type *m_type;
  
public:
  
};

class TStruct : Type {
  std::vector<Field> m_fields;

public:
  TStruct(const std::vector<Field> &fields, required bool)
    : Type(Type::Kind::STRUCT, required) {}
};

class TArray : Type {
  const Type *m_element_type;
  
public:
  TArray(Type *element_type, bool required)
    : Type(Type::Kind::ARRAY, required),
      m_element_type(element_type) {}
  
  const Type *element_type() const { return m_element_type; }
};

