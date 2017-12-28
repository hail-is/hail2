
#include <sstream>

#include "region.hh"

namespace hail {

std::ostream &
operator<<(std::ostream &out, const TypedRegionValue &t) {
  t.put_to(out);
  return out;
}

std::ostream &
TypedRegionValue::put_to(std::ostream &out, uint64_t off, const Type *t) const {
  switch (t->kind) {
  case BaseType::Kind::BOOLEAN:
    if (region->load_bool(off))
      out << "true";
    else
      out << "false";
    break;
  case BaseType::Kind::INT32:
    out << region->load_int(off);
    break;
  case BaseType::Kind::INT64:
    out << region->load_long(off);
    break;
  case BaseType::Kind::FLOAT32:
    out << region->load_float(off);
    break;
  case BaseType::Kind::FLOAT64:
    out << region->load_double(off);
    break;
  case BaseType::Kind::STRING:
    {
      uint64_t soff = region->load_offset(off);
      uint32_t n = region->load_int(soff);
      out << std::string((const char *)(region->mem + soff + 4), n);
    }
    break;
  case BaseType::Kind::STRUCT:
    {
      const TStruct *ts = cast<TStruct>(t);
      for (uint64_t i = 0; i < ts->fields.size(); ++i) {
	if (i > 0)
	  out << ", ";
	out << ts->fields[i].name << ": ";
	if (region->is_field_defined(ts, off, i))
	  put_to(out, off + ts->field_offset[i], ts->fields[i].type);
	else
	  out << ".";
      }
    }
    break;
  case BaseType::Kind::ARRAY:
    {
      const TArray *ta = cast<TArray>(t);
      uint64_t aoff = region->load_offset(off);
      uint32_t n = region->load_int(aoff);
      uint64_t elements_off = aoff + ta->elements_offset(n);
      uint64_t element_size = ta->element_size();
      out << "[" << n << "; ";
      for (uint64_t i = 0; i < n; ++i) {
	if (i > 0)
	  out << ", ";
	if (region->is_element_defined(ta, aoff, i))
	  put_to(out, elements_off + i*element_size, ta->element_type);
	else
	  out << ".";
      }
    }
    break;
  default: abort();
  }
  return out;
}

std::ostream &
TypedRegionValue::put_to(std::ostream &out) const {
  put_to(out, offset, type->fundamental_type);
  return out;
}

std::string
TypedRegionValue::to_string() const {
  std::ostringstream ss;
  ss << *this;
  return ss.str();
}

} // namespace hail
