#ifndef HAIL_CONTEXT_HH
#define HAIL_CONTEXT_HH
#pragma once

#include <unordered_set>
#include <rapidjson/document.h>

#include "type.hh"

namespace hail {

class TypeLexer;

class Context {
  static Context *context;
  
  const TBoolean boolean_required, boolean_optional;
  const TInt32 int32_required, int32_optional;
  const TInt64 int64_required, int64_optional;
  const TFloat32 float32_required, float32_optional;
  const TFloat64 float64_required, float64_optional;
  const TString string_required, string_optional;
  const TCall *call_required, *call_optional;
  const TAltAllele *alt_allele_required, *alt_allele_optional;
  
  std::unordered_set<const BaseType *,
		     hash_points_to<BaseType>,
		     equal_to_points_to<BaseType>> types;
  
  // FIXME sublcass of BaseType
  // FIXME actually intern
  template<typename T> const T *intern(const T *t);
  
public:
  Context();
  ~Context();
  
  static Context &the_context() { return *context; }
  
  const TBoolean *boolean_type(bool required) {
    if (required)
      return &boolean_required;
    else
      return &boolean_optional;
  }
  
  const TInt32 *int32_type(bool required) {
    if (required)
      return &int32_required;
    else
      return &int32_optional;
  }
  
  const TInt64 *int64_type(bool required) {
    if (required)
      return &int64_required;
    else
      return &int64_optional;
  }
  
  const TFloat32 *float32_type(bool required) {
    if (required)
      return &float32_required;
    else
      return &float32_optional;
  }
  
  const TFloat64 *float64_type(bool required) {
    if (required)
      return &float64_required;
    else
      return &float64_optional;
  }
  
  const TString *string_type(bool required) {
    if (required)
      return &string_required;
    else
      return &string_optional;
  }
  
  const TCall *call_type(bool required) {
    if (required)
      return call_required;
    else
      return call_optional;
  }
  
  const TAltAllele *alt_allele_type(bool required) {
    if (required)
      return alt_allele_required;
    else
      return alt_allele_optional;
  }
  
  const TStruct *struct_type(const std::vector<Field> &fields, bool required);
  const TArray *array_type(const Type *element_type, bool required);
  const TSet *set_type(const Type *element_type, bool required);
  const TLocus *locus_type(const std::string &gr, bool required);
  const TVariant *variant_type(const std::string &gr, bool required);
  
  const TMatrixTable *matrix_table_type(const rapidjson::Document &d);
  
  const Type *parse_type(TypeLexer &lexer);
  const Type *parse_type(const char *s);
  
  // FIXME private?  move?
  const Type *alt_allele_representation(bool required);
  const Type *locus_representation(bool required);
  const Type *variant_representation(bool required);
};

// FIXME
inline Context &the_context() {
  return Context::the_context();
}

} // namespace hail

#endif // HAIL_CONTEXT_HH
