
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <limits>
#include <ctype.h>

#include <fmt/format.h>

#include "casting.hh"
#include "type.hh"

std::ostream &
operator <<(std::ostream &out, const BaseType &t) {
  t.put_to(out);
  return out;
}

class ParseError : public std::runtime_error {
public:
  ParseError(const std::string &what)
    : std::runtime_error(what) {}
  ParseError(const char *what)
    : std::runtime_error(what) {}
};

enum Token {
  BOOLEAN = std::numeric_limits<char>::max() + 1,
  INT32, INT64, FLOAT32, FLOAT64, STRING, ARRAY, SET, STRUCT, EMPTY,
  CALL, LOCUS, ALTALLELE, VARIANT,
  ID
};

class TypeLexer {
  // FIXME
  static std::unordered_map<std::string, int> keywords;
  
  const char *s;
  std::string text_;
  
public:
  TypeLexer(const char *s)
    : s(s) {}
  
  std::string text() const { return text_; }
  std::string token_description(int t) const;
  
  int lex();
  void expect(int expected);
};

std::unordered_map<std::string, int> TypeLexer::keywords = {
  { "Boolean", BOOLEAN },
  { "Int32", INT32 },
  { "Int64", INT64 },
  { "Float32", FLOAT32 },
  { "Float64", FLOAT64 },
  { "String", STRING },
  { "Array", ARRAY },
  { "Set", SET },
  { "Struct", STRUCT },
  { "Empty", EMPTY },
  { "Call", CALL },
  { "Locus", LOCUS },
  { "AltAllele", ALTALLELE },
  { "Variant", VARIANT },
};

int
TypeLexer::lex() {
  while (isspace(*s))
    ++s;
  
  if (isalpha(*s) || *s == '_') {
    const char *b = s;
    while (isalnum(*s) || *s == '_')
      ++s;
    
    text_.assign(b, s);
    auto i = keywords.find(text_);
    if (i != keywords.cend())
      return i->second;
    
    return ID;
  }

  char c = *s;
  ++s;
  return c;
}

std::string
TypeLexer::token_description(int t) const {
  switch (t) {
  case BOOLEAN:  return "Boolean";
  case INT32:  return "Int32";
  case INT64:  return "Int64";
  case FLOAT32:  return "Float32";
  case FLOAT64:  return "Float64";
  case STRING:  return "String";
  case ARRAY:  return "Array";
  case SET:  return "Set";
  case STRUCT:  return "Struct";
  case EMPTY:  return "Empty";
  case ID: return "identifier";
  case '\0': return "end of input";
  default:
    char buf[2] = { (char)t, '\0' };
    return buf;
  }
}

void
TypeLexer::expect(int expected) {
  int found = lex();
  if (found != expected)
    throw ParseError(fmt::format("expected {}", token_description(expected)));
}

static std::shared_ptr<Type>
parse_type(TypeLexer &lexer) {
  int t = lexer.lex();
  bool required = false;
  if (t == '!') {
    required = true;
    t = lexer.lex();
  }
  switch(t) {
  case BOOLEAN:
    return std::make_shared<TBoolean>(required);
  case INT32:
    return std::make_shared<TInt32>(required);
  case INT64:
    return std::make_shared<TInt64>(required);
  case FLOAT32:
    return std::make_shared<TFloat32>(required);
  case FLOAT64:
    return std::make_shared<TFloat64>(required);
  case STRING:
    return std::make_shared<TString>(required);
  case ARRAY: {
    lexer.expect('[');
    auto element_type = parse_type(lexer);
    lexer.expect(']');
    return std::make_shared<TArray>(element_type, required);
  }
  case SET: {
    lexer.expect('[');
    auto element_type = parse_type(lexer);
    lexer.expect(']');
    return std::make_shared<TSet>(element_type, required);
  }
  case STRUCT: {
    lexer.expect('{');
    t = lexer.lex();
    std::vector<Field> fields;
    while (t == ID) {
      std::string name = lexer.text();
      lexer.expect(':');
      auto type = parse_type(lexer);
      fields.push_back(Field { name, type });
      t = lexer.lex();
      if (t == ',')
	t = lexer.lex();
    }
    if (t != '}')
      throw ParseError("expected }");
    return std::make_shared<TStruct>(fields, required);
  }
  case EMPTY:
    return std::make_shared<TStruct>(std::vector<Field>(), required);
  case CALL:
    return std::make_shared<TCall>(required);
  case LOCUS: {
    lexer.expect('(');
    lexer.expect(ID);
    std::string gr = lexer.text();
    lexer.expect(')');
    return std::make_shared<TLocus>(gr, required);
  }
  case ALTALLELE:
    return std::make_shared<TAltAllele>(required);
  case VARIANT: {
    lexer.expect('(');
    lexer.expect(ID);
    std::string gr = lexer.text();
    lexer.expect(')');
    return std::make_shared<TVariant>(gr, required);
  }
  default:
    throw ParseError(fmt::format("parse error at {}", lexer.text()));
  }
}

std::shared_ptr<Type>
parse_type(const char *s) {
  TypeLexer lexer(s);
  auto t = parse_type(lexer);
  lexer.expect('\0');
  return t;
}

BaseType::~BaseType() {}

std::string
BaseType::to_string() const {
  std::ostringstream ss;
  ss << *this;
  return ss.str();
}

TMatrixTable::TMatrixTable(const rapidjson::Document &d)
  : BaseType(Kind::MATRIXTABLE),
    global_type(parse_type(d["global_schema"].GetString())),
    col_key_type(parse_type(d["sample_schema"].GetString())),
    col_type(parse_type(d["sample_annotation_schema"].GetString())),
    row_key_type(parse_type(d["variant_schema"].GetString())),
    row_type(parse_type(d["variant_annotation_schema"].GetString())),
    entry_type(parse_type(d["genotype_schema"].GetString())),
    row_impl_type(std::make_shared<TStruct>(
		    std::vector<Field> {
		      { "pk", row_partition_key_type() },
			{ "v", row_key_type },
			  { "va", row_type },
			    { "gs", std::make_shared<TArray>(entry_type, false) }
		    },
		    false)) {}

std::shared_ptr<Type>
TMatrixTable::row_partition_key_type() const {
  if (auto tv = dyn_cast<TVariant>(row_key_type))
    return std::make_shared<TLocus>(tv->gr, tv->required);
  else
    return row_key_type;
}

std::ostream &
TMatrixTable::put_to(std::ostream &out) const {
  out << "MatrixTable {\n";
  out << "  global " << *global_type << "\n";
  out << "  col_key " << *col_key_type << "\n";
  out << "  col " << *col_type << "\n";
  out << "  row_key " << *row_key_type << "\n";
  out << "  row " << *row_type << "\n";
  out << "  entry " << *entry_type << "\n";

  // FIXME remove
  out << "  row_impl " << *row_impl_type << "\n";
  
  out << "}\n";
  return out;
}

std::ostream &
TBoolean::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Boolean";
  return out;
}

std::ostream &
TInt32::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Int32";
  return out;
}

std::ostream &
TInt64::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Int64";
  return out;
}

std::ostream &
TFloat32::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Float32";
  return out;
}

std::ostream &
TFloat64::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Float64";
  return out;
}

std::ostream &
TString::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "String";
  return out;
}

TStruct::TStruct(const std::vector<Field> &fields, bool required)
  : Type(Kind::STRUCT, required),
    fields(fields),
    field_offset(fields.size()),
    n_nonrequired_fields(0),
    field_missing_bit(fields.size())
{
  // FIXME incompatible with JVM code
  for (uint64_t i = 0; i < fields.size(); ++i) {
    if (!fields[i].type->required) {
      field_missing_bit[i] = n_nonrequired_fields;
      ++n_nonrequired_fields;
    }
  }
  
  uint64_t align = 1;
  uint64_t off = missing_bits_size();
  for (uint64_t i = 0; i < fields.size(); ++i) {
    uint64_t a = fields[i].type->alignment();
    
    off = alignto(off, a);
    field_offset[i] = off;
    off += fields[i].type->size();
    
    if (a > align)
      align = a;
  }
  alignment_ = align;
  size_ = off;
}

std::ostream &
TStruct::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Struct { ";
  for (auto &f : fields)
    out << f.name << ": " << *f.type << ", ";
  out << "}";
  return out;
}

std::ostream &
TArray::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  return out << "Array[" << *element_type << "]";
}

std::ostream &
TSet::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  return out << "Set[" << *element_type << "]";
}

std::ostream &
TCall::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Call";
  return out;
}

std::ostream &
TLocus::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Locus(" << gr << ")";
  return out;
}

std::ostream &
TAltAllele::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "AltAllele";
  return out;
}

std::ostream &
TVariant::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Variant(" << gr << ")";
  return out;
}
