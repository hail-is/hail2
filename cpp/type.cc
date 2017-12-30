
#include <ctype.h>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <limits>
#include <algorithm>

#include <fmt/format.h>
#include <rapidjson/document.h>

#include "casting.hh"
#include "type.hh"
#include "context.hh"

namespace hail {

std::ostream &
operator<<(std::ostream &out, const BaseType &t) {
  t.put_to(out);
  return out;
}

BaseType::~BaseType() {}

std::size_t
BaseType::hash() const {
  return std::hash<Kind>{}(kind);
}
  
bool
BaseType::operator==(const BaseType &that) const {
  return kind == that.kind;
}

std::string
BaseType::to_string() const {
  std::ostringstream ss;
  ss << *this;
  return ss.str();
}

TMatrixTable::TMatrixTable(Context &c,
			   const Type *global_type,
			   const Type *col_key_type,
			   const Type *col_type,
			   const Type *row_key_type,
			   const Type *row_type,
			   const Type *entry_type)
  : BaseType(Kind::MATRIXTABLE),
    global_type(global_type),
    col_key_type(col_key_type),
    col_type(col_type),
    row_key_type(row_key_type),
    row_type(row_type),
    entry_type(entry_type)
{
  const Type *row_pk_type;
  if (auto tv = dyn_cast<TVariant>(row_key_type))
    row_pk_type = c.locus_type(tv->gr, tv->required);
  else
    row_pk_type = row_key_type;
  
  row_impl_type = c.struct_type(std::vector<Field> {
      { "pk", row_pk_type },
	{ "v", row_key_type },
	  { "va", row_type },
	    { "gs", c.array_type(entry_type, false) }
    },
    false);
}

std::size_t
TMatrixTable::hash() const {
  std::size_t h = BaseType::hash();
  hash_combine<BaseType>(h, *global_type);
  hash_combine<BaseType>(h, *col_key_type);
  hash_combine<BaseType>(h, *col_type);
  hash_combine<BaseType>(h, *row_key_type);
  hash_combine<BaseType>(h, *row_type);
  hash_combine<BaseType>(h, *entry_type);
  return h;
}

bool
TMatrixTable::operator==(const BaseType &that) const {
  auto *that2 = dyn_cast<TMatrixTable>(&that);
  return that2
    && *global_type == *that2->global_type
    && *col_key_type == *that2->col_key_type
    && *col_type == *that2->col_type
    && *row_key_type == *that2->row_key_type
    && *row_type == *that2->row_type
    && *entry_type == *that2->entry_type;
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

Type::Type(Kind kind, bool required)
  : BaseType(kind), required(required) {
}

Type::Type(Kind kind, bool required, uint64_t alignment, uint64_t size)
  : BaseType(kind), required(required),
    alignment(alignment), size(size) {
}

Type::Type(Kind kind, bool required, uint64_t alignment, uint64_t size, const Type *fundamental_type)
  : BaseType(kind), required(required),
    alignment(alignment), size(size),
    fundamental_type(fundamental_type) {
}

std::size_t
Type::hash() const {
  std::size_t h = BaseType::hash();
  hash_combine(h, required);
  return h;
}

bool
Type::operator==(const BaseType &that) const {
  return kind == that.kind
    && required == static_cast<const Type &>(that).required;
}

TBoolean::TBoolean(bool required)
  : Type(Kind::BOOLEAN, required, 1, 1, this) {}

std::ostream &
TBoolean::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Boolean";
  return out;
}

TInt32::TInt32(bool required)
  : Type(Kind::INT32, required, 4, 4, this) {}

std::ostream &
TInt32::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Int32";
  return out;
}

TInt64::TInt64(bool required)
  : Type(Kind::INT64, required, 8, 8, this) {}

std::ostream &
TInt64::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Int64";
  return out;
}

TFloat32::TFloat32(bool required)
  : Type(Kind::INT32, required, 4, 4, this) {}

std::ostream &
TFloat32::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Float32";
  return out;
}

TFloat64::TFloat64(bool required)
  : Type(Kind::FLOAT64, required, 8, 8, this) {}

std::ostream &
TFloat64::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Float64";
  return out;
}

TString::TString(bool required)
  : Type(Kind::STRING, required, 8, 8, this) {}

std::ostream &
TString::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "String";
  return out;
}

std::size_t
Field::hash() const {
  std::size_t h = std::hash<std::string>{}(name);
  hash_combine<BaseType>(h, *type);
  return h;
}

bool
Field::operator==(const Field &f) const {
  return name == f.name
    && *type == *f.type;
}

TStruct::TStruct(Context &c, const std::vector<Field> &fields, bool required)
  : Type(Kind::STRUCT, required),
    fields(fields),
    field_offset(fields.size()),
    n_nonrequired_fields(0),
    field_missing_bit(fields.size()) {
  // FIXME incompatible with JVM code
  for (uint64_t i = 0; i < fields.size(); ++i) {
    if (!fields[i].type->required) {
      field_missing_bit[i] = n_nonrequired_fields;
      ++n_nonrequired_fields;
    }
  }
  
  alignment = 1;
  size = missing_bits_size();
  for (uint64_t i = 0; i < fields.size(); ++i) {
    uint64_t a = fields[i].type->alignment;
    
    size = alignto(size, a);
    field_offset[i] = size;
    size += fields[i].type->size;
    
    if (a > alignment)
      alignment = a;
  }
  
  if (std::all_of(fields.begin(), fields.end(),
		  [](const Field &f) { return f.type->is_fundamental(); }))
    fundamental_type = this;
  else {
    std::vector<Field> fundamental_fields(fields.size());
    std::transform(fields.begin(), fields.end(),
		   fundamental_fields.begin(),
		   [](const Field &f) { return Field { f.name, f.type->fundamental_type }; });
    fundamental_type = c.struct_type(fundamental_fields, required);
  }
}

std::size_t
TStruct::hash() const {
  std::size_t h = Type::hash();
  hash_combine<std::vector<Field>>(h, fields);
  return h;
}

bool
TStruct::operator==(const BaseType &that) const {
  return Type::operator==(that)
    && fields == cast<TStruct>(that).fields;
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

TArray::TArray(Context &c, const Type *element_type, bool required)
  : Type(Kind::ARRAY, required, 8, 8),
    element_type(element_type) {
  if (element_type->is_fundamental())
    fundamental_type = this;
  else
    fundamental_type = c.array_type(element_type->fundamental_type, required);
}

std::size_t
TArray::hash() const {
  std::size_t h = Type::hash();
  hash_combine<BaseType>(h, *element_type);
  return h;
}

bool
TArray::operator==(const BaseType &that) const {
  return Type::operator==(that)
    && *element_type == *cast<TArray>(that).element_type;
}

std::ostream &
TArray::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  return out << "Array[" << *element_type << "]";
}

TComplex::TComplex(const Type *representation, Kind kind, bool required)
  : Type(kind, required, representation->alignment, representation->size, representation->fundamental_type),
    representation(representation) {}

TSet::TSet(Context &c, const Type *element_type, bool required)
  : TComplex(c.array_type(element_type, required), Kind::SET, required),
    element_type(element_type)
{
}

std::size_t
TSet::hash() const {
  std::size_t h = Type::hash();
  hash_combine<BaseType>(h, *element_type);
  return h;
}

bool
TSet::operator==(const BaseType &that) const {
  return Type::operator==(that)
    && *element_type == *cast<TSet>(that).element_type;
}

std::ostream &
TSet::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  return out << "Set[" << *element_type << "]";
}

TCall::TCall(Context &c, bool required)
  : TComplex(c.int32_type(required), Kind::CALL, required) {}

std::ostream &
TCall::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Call";
  return out;
}

TLocus::TLocus(Context &c, const std::string &gr, bool required)
  : TComplex(c.locus_representation(required), Kind::LOCUS, required),
    gr(gr) {}

std::size_t
TLocus::hash() const {
  std::size_t h = Type::hash();
  hash_combine(h, gr);
  return h;
}

bool
TLocus::operator==(const BaseType &that) const {
  return Type::operator==(that)
    && gr == cast<TLocus>(that).gr;
}

std::ostream &
TLocus::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Locus(" << gr << ")";
  return out;
}

TAltAllele::TAltAllele(Context &c, bool required)
  : TComplex(c.alt_allele_representation(required), Kind::ALTALLELE, required) {}

std::ostream &
TAltAllele::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "AltAllele";
  return out;
}

TVariant::TVariant(Context &c, const std::string &gr, bool required)
  : TComplex(c.variant_representation(required), Kind::VARIANT, required),
    gr(gr) {}

std::size_t
TVariant::hash() const {
  std::size_t h = Type::hash();
  hash_combine(h, gr);
  return h;
}

bool
TVariant::operator==(const BaseType &that) const {
  return Type::operator==(that)
    && gr == cast<TVariant>(that).gr;
}

std::ostream &
TVariant::put_to(std::ostream &out) const {
  if (required)
    out << "!";
  out << "Variant(" << gr << ")";
  return out;
}

} // namespace hail
