
#include <unordered_map>
#include <fmt/format.h>

#include "type.hh"
#include "context.hh"

namespace hail {

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

Context *Context::context;

template<typename T> const T *
Context::intern(const T *t) {
  const BaseType *bt = static_cast<const BaseType *>(t);
  auto p = types.insert(bt);
  if (p.second)
    return t;
  else {
    delete t;
    return static_cast<const T *>(*p.first);
  }
}

Context::Context()
  : boolean_required(true), boolean_optional(false),
    int32_required(true), int32_optional(false),
    int64_required(true), int64_optional(false),
    float32_required(true), float32_optional(false),
    float64_required(true), float64_optional(false),
    string_required(true), string_optional(false) {
  assert(context == nullptr);
  context = this;
  
  call_required = new TCall(*this, true);
  call_optional = new TCall(*this, false);
  
  alt_allele_required = new TAltAllele(*this, true);
  alt_allele_optional = new TAltAllele(*this, false);
}

Context::~Context() {
  context = nullptr;
}

const Type *
Context::parse_type(TypeLexer &lexer) {
  int t = lexer.lex();
  bool required = false;
  if (t == '!') {
    required = true;
    t = lexer.lex();
  }
  switch(t) {
  case BOOLEAN:
    return boolean_type(required);
  case INT32:
    return int32_type(required);
  case INT64:
    return int64_type(required);
  case FLOAT32:
    return float32_type(required);
  case FLOAT64:
    return float64_type(required);
  case STRING:
    return string_type(required);
  case ARRAY: {
    lexer.expect('[');
    auto element_type = parse_type(lexer);
    lexer.expect(']');
    return array_type(element_type, required);
  }
  case SET: {
    lexer.expect('[');
    auto element_type = parse_type(lexer);
    lexer.expect(']');
    return set_type(element_type, required);
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
    return struct_type(fields, required);
  }
    // FIXME delete
  case EMPTY:
    return struct_type(std::vector<Field>(), required);
  case CALL:
    return call_type(required);
  case LOCUS: {
    lexer.expect('(');
    lexer.expect(ID);
    std::string gr = lexer.text();
    lexer.expect(')');
    return locus_type(gr, required);
  }
  case ALTALLELE:
    return alt_allele_type(required);
  case VARIANT: {
    lexer.expect('(');
    lexer.expect(ID);
    std::string gr = lexer.text();
    lexer.expect(')');
    return variant_type(gr, required);
  }
  default:
    throw ParseError(fmt::format("parse error at {}", lexer.text()));
  }
}

const Type *
Context::parse_type(const char *s) {
  TypeLexer lexer(s);
  auto t = parse_type(lexer);
  lexer.expect('\0');
  return t;
}

const TStruct *
Context::struct_type(const std::vector<Field> &fields, bool required) {
  return intern(new TStruct(*this, fields, required));
}

const TArray *
Context::array_type(const Type *element_type, bool required) {
  return intern(new TArray(*this, element_type, required));
}

const TSet *
Context::set_type(const Type *element_type, bool required) {
  return intern(new TSet(*this, element_type, required));
}

const TLocus *
Context::locus_type(const std::string &gr, bool required) {
  return intern(new TLocus(*this, gr, required));
}

const TVariant *
Context::variant_type(const std::string &gr, bool required) {
  return intern(new TVariant(*this, gr, required));
}

const TMatrixTable *
Context::matrix_table_type(const rapidjson::Document &d) {
  return intern(new TMatrixTable(
		  *this,
		  parse_type(d["global_schema"].GetString()),
		  parse_type(d["sample_schema"].GetString()),
		  parse_type(d["sample_annotation_schema"].GetString()),
		  parse_type(d["variant_schema"].GetString()),
		  parse_type(d["variant_annotation_schema"].GetString()),
		  parse_type(d["genotype_schema"].GetString())));
}

const Type *
Context::alt_allele_representation(bool required) {
  return struct_type(std::vector<Field> {
      Field { "ref", string_type(true) },
	Field { "alt", string_type(true) }
    }, required);
}

const Type *
Context::locus_representation(bool required) {
  return struct_type(std::vector<Field> {
      Field { "contig", string_type(true) },
	Field { "pos", int32_type(true) }
    }, required);
}

const Type *
Context::variant_representation(bool required) {
  return struct_type(std::vector<Field> {
      Field { "contig", string_type(true) },
	Field { "pos", int32_type(true) },
	  Field { "ref", string_type(true) },
	    Field { "altAlleles", array_type(alt_allele_type(true), true) }
    }, required);
}

} // namespace hail
