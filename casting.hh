#ifndef HAIL_CASTING_HH
#define HAIL_CASTING_HH

template<class T> bool
isa(const typename T::Base *v) {
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
    return 0;
}

#endif
