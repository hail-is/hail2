
#include <iostream>

#include "type.hh"

std::ostream &
operator <<(std::ostream &out, const BaseType &t) {
  t.put_to(out);
  return out;
}

std::ostream &
operator<<(std::ostream &out, TypeToken token) {
  switch (token) {
  case TypeToken::BOOLEAN: out << "Boolean"; break;
  case TypeToken::INT32: out << "Int32"; break;
  case TypeToken::INT64: out << "Int64"; break;
  case TypeToken::FLOAT32: out << "Float32"; break;
  case TypeToken::FLOAT64: out << "Float64"; break;
  case TypeToken::STRING: out << "String"; break;
  case TypeToken::ARRAY: out << "Array"; break;
  case TypeToken::STRUCT: out << "Struct"; break;
  case TypeToken::ID: out << "<id>"; break;
  case TypeToken::LBRACE: out << "{"; break;
  case TypeToken::RBRACE: out << "}"; break;
  case TypeToken::LBRACKET: out << "["; break;
  case TypeToken::RBRACKET: out << "]"; break;
  case TypeToken::COLON: out << ":"; break;
  case TypeToken::COMMA: out << ","; break;
  case TypeToken::BANG: out << "!"; break;
  default: out << "???"; break;
  }
  return out;
}

std::vector<std::tuple<std::regex, TypeToken>>
TokenIterator::patterns = {
  std::make_tuple(std::regex("^Boolean", std::regex::ECMAScript), TypeToken::BOOLEAN),
  std::make_tuple(std::regex("^Int32", std::regex::ECMAScript), TypeToken::INT32),
  std::make_tuple(std::regex("^Int64", std::regex::ECMAScript), TypeToken::INT64),
  std::make_tuple(std::regex("^Float32", std::regex::ECMAScript), TypeToken::FLOAT32),
  std::make_tuple(std::regex("^Float64", std::regex::ECMAScript), TypeToken::FLOAT64),
  std::make_tuple(std::regex("^String", std::regex::ECMAScript), TypeToken::STRING),
  std::make_tuple(std::regex("^Array", std::regex::ECMAScript), TypeToken::ARRAY),
  std::make_tuple(std::regex("^Struct", std::regex::ECMAScript), TypeToken::STRUCT),
  std::make_tuple(std::regex(R"(^[a-zA-Z_]\w*)", std::regex::ECMAScript), TypeToken::ID),
  std::make_tuple(std::regex(R"(^\{)", std::regex::ECMAScript), TypeToken::LBRACE),
  std::make_tuple(std::regex(R"(^\})", std::regex::ECMAScript), TypeToken::RBRACE),
  std::make_tuple(std::regex(R"(^\[)", std::regex::ECMAScript), TypeToken::LBRACKET),
  std::make_tuple(std::regex(R"(^\])", std::regex::ECMAScript), TypeToken::RBRACKET),
  std::make_tuple(std::regex("^:", std::regex::ECMAScript), TypeToken::COLON),
  std::make_tuple(std::regex("^,", std::regex::ECMAScript), TypeToken::COMMA),
  std::make_tuple(std::regex("^!", std::regex::ECMAScript), TypeToken::BANG),
};

std::shared_ptr<Type>
parse_type(TokenIterator &i) {
  // FIXME error checking
  auto t = *i;
  ++i;
  switch (t) {
  case TypeToken::BOOLEAN: return std::make_shared<TBoolean>(false);
  case TypeToken::INT32: return std::make_shared<TInt32>(false);
    // case TypeToken::INT64: return std::make_shared<TInt64>(false);
    // case TypeToken::FLOAT32: return std::make_shared<TFloat32>(false);
  case TypeToken::FLOAT64: return std::make_shared<TFloat64>(false);
  case TypeToken::ARRAY: {
    ++i; // [
    auto et = parse_type(i);
    ++i; // ]
    return std::make_shared<TArray>(et, false);
  }
  case TypeToken::STRUCT: {
    ++i; // {
    std::vector<Field> fields;
    if (*i == TypeToken::ID) {
      auto name = i.text();
      ++i; // id
      ++i; // :
      auto ft = parse_type(i);
      fields.push_back(Field { name, ft });
    }
    ++i; // }
    return std::make_shared<TStruct>(fields, false);
  }
  default:
    std::cerr << "unexpected token" << t << "\n";
    abort();
  }
}

std::shared_ptr<Type>
parse_type(const char *s) {
  TokenIterator i(s);
  return parse_type(i);
}


BaseType::~BaseType() {}

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
TFloat64::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Float64";
  return out;
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
