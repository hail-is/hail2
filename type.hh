
#include <ctype.h>

#include <string>
#include <vector>
#include <ostream>
#include <memory>
#include <regex>
#include <cassert>

class BaseType;
class Type;

extern std::ostream &operator<<(std::ostream &out, const BaseType &t);

extern std::shared_ptr<Type> parse_type(const char *s);

enum class TypeToken {
  BOOLEAN,
  INT32,
  INT64,
  FLOAT32,
  FLOAT64,
  STRING,
  ARRAY,
  STRUCT,
  ID,
  LBRACE,
  RBRACE,
  LBRACKET,
  RBRACKET,
  COLON,
  COMMA,
  BANG,
};

extern std::ostream &operator<<(std::ostream &out, TypeToken token);

class TokenIterator {
  static std::vector<std::tuple<std::regex, TypeToken>> patterns;
  
  const char *b;
  
  const char *token_e;
  TypeToken token;
  
  void skip_ws() {
    while (*b && isspace(*b))
      ++b;
  }
  
  void lex_token() {
    for (auto &p : patterns) {
      std::cmatch m;
      if (std::regex_search(b, m, std::get<0>(p))) {
        assert(m.position() == 0);
        token_e = b + m.length();
        token = std::get<1>(p);
        return;
      }
    }
    abort(); // no match
  }
  
  void advance() {
    skip_ws();
    if (!at_end())
      lex_token();
  }
  
public:
  TokenIterator(const char *b)
    : b(b) {
    advance();
  }
  
  TokenIterator &operator++() {
    b = token_e;
    advance();
    return *this;
  }
  
  TokenIterator operator++(int) {
    operator++();
    return *this;
  }
  
  TypeToken operator*() const {
    assert(!at_end());
    return token;
  }
  
  bool operator==(const TokenIterator &that) {
    return b == that.b;
  }
  bool operator !=(const TokenIterator &that) {
    return !operator==(that);
  }

  std::string text() const {
    return std::string(b, token_e);
  }

  bool at_end() const { return *b == '\0'; }
};

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
