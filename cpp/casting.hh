#ifndef HAIL_CASTING_HH
#define HAIL_CASTING_HH
#pragma once

#include <memory>
#include <type_traits>

namespace hail {

template<class T, class U>
struct propagate_const {
  using type = typename std::conditional<std::is_const<T>::value, std::add_const<U>, U>::type;
};

template<class T, class U>
struct propagate_volatile {
  using type = typename std::conditional<std::is_volatile<T>::value, std::add_volatile<U>, U>::type;
};

template<class T, class U>
struct propagate_cv {
  using type = typename propagate_volatile<T, typename propagate_const<T, U>::type>::type;
};

template<class T> bool
isa(const typename T::Base *v) {
  return v->kind == T::kindof;
}

template<class T> bool
isa(std::shared_ptr<const typename T::Base> v) {
  return v->kind == T::kindof;
}

template<class T> const T *
cast(const typename T::Base *v) {
  assert (isa<T>(v));
  return static_cast<const T *>(v);
}

template<class T> T *
cast(typename T::Base *v) {
  assert (isa<T>(v));
  return static_cast<T *>(v);
}

template<class T, class U> typename std::enable_if<std::is_base_of<typename T::Base, U>::value,
						   std::shared_ptr<typename propagate_cv<U, T>::type>>::type
cast(std::shared_ptr<U> v) {
  assert (isa<T>(v));
  return std::static_pointer_cast<typename propagate_cv<U, T>::type>(v);
}

template<class T> const T *
dyn_cast(const typename T::Base *v) {
  if (isa<T>(v))
    return static_cast<const T *>(v);
  else
    return 0;
}

template<class T> T *
dyn_cast(typename T::Base *v) {
  if (isa<T>(v))
    return static_cast<T *>(v);
  else
    return nullptr;
}

template<class T, class U> typename std::enable_if<std::is_base_of<typename T::Base, U>::value,
						   std::shared_ptr<typename propagate_cv<U, T>::type>>::type
dyn_cast(std::shared_ptr<U> v) {
  if (isa<T>(v))
    return std::static_pointer_cast<typename propagate_cv<U, T>::type>(v);
  else
    return nullptr;
}

} // namespace hail

#endif // HAIL_CASTING_HH
