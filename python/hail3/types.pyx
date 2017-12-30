# distutils: language=c++
# cython: language_level=3
from libcpp cimport bool
from libcpp.memory cimport shared_ptr, make_shared
from libcpp.string cimport string
from libc.stdint cimport uintptr_t

from hail3 cimport libhail

cdef class Context(object):
    cdef libhail.Context *context
    cdef dict _types

    def __cinit__(self):
        self.context = new libhail.Context()
        self._types = {}

        print('''Welcome to
      __  __      <>_______
     / / / /___  __/ /__  /
    / /_/ / __ `/ / / /_ < 
   / __  / /_/ / / /___/ / 
  /_/ /_/\__,_/_/_//____/   version TODO''')

    def read(self, filename):
        return MatrixTable(self, filename)
  
    cdef _get_type(self, const libhail.BaseType *ct):
        cdef uintptr_t h = <uintptr_t>ct
        if h in self._types:
            return self._types[h]
        if ct.kind == libhail.MATRIXTABLE:
            t = TMatrixTable_init(<const libhail.TMatrixTable *>ct)
        else:
            t = Type_init(<const libhail.Type *>ct)
        self._types[h] = t
        return t

    def boolean_type(self, bool required):
        return self._get_type(self.context.boolean_type(required))

    def int32_type(self, bool required):
        return self._get_type(self.context.int32_type(required))

    def int64_type(self, bool required):
        return self._get_type(self.context.int64_type(required))

    def float32_type(self, bool required):
        return self._get_type(self.context.float32_type(required))

    def float64_type(self, bool required):
        return self._get_type(self.context.float64_type(required))

    def string_type(self, bool required):
        return self._get_type(self.context.string_type(required))

    def call_type(self, bool required):
        return self._get_type(self.context.call_type(required))

    def alt_allele_type(self, bool required):
        return self._get_type(self.context.alt_allele_type(required))

cdef TypedRegionValue_init(libhail.TypedRegionValue ctrv):
    trv = TypedRegionValue()
    trv.ctrv = ctrv
    return trv

cdef class TypedRegionValue(object):
    cdef libhail.TypedRegionValue ctrv

    def __repr__(self):
        return self.ctrv.to_string().decode('ascii')

cdef MatrixTableIterator_init(shared_ptr[libhail.MatrixTableIterator] ci):
  i = MatrixTableIterator()
  i.ci = ci
  return i
    
cdef class MatrixTableIterator(object):
    cdef shared_ptr[libhail.MatrixTableIterator] ci

    def __iter__(self):
        return self

    def __next__(self):
        if self.ci.get().has_next():
            return TypedRegionValue_init(self.ci.get().next())
        else:
            raise StopIteration()

cdef class MatrixTable(object):
    cdef Context context
    cdef shared_ptr[libhail.MatrixTable] mt

    def __init__(self, Context c, str filename):
        self.context = c
        self.mt = make_shared[libhail.MatrixTable](c.context[0], <string>filename.encode('ascii'))

    # FIXME leaves file open
    def rows(self):
        return MatrixTableIterator_init(self.mt.get().iterator())

    def count_rows(self):
        return self.mt.get().count_rows()

    @property
    def typ(self):
        return self.context._get_type(self.mt.get().typ)

cdef class BaseType:
    cdef const libhail.BaseType *ct

    def __hash__(self):
        return <uintptr_t>self.ct

    def __eq__(self, BaseType that):
        return self.ct == that.ct

    def __repr__(BaseType self):
        return self.ct.to_string().decode('ascii')

cdef Type_init(const libhail.Type *ct):
    t = Type()
    t.ct = ct
    return t

cdef class Type(BaseType):
    cdef const libhail.Type *ctype(self):
        return <const libhail.Type *>self.ct

    @property
    def required(self):
        return self.ctype().required

cdef TMatrixTable_init(const libhail.TMatrixTable *ct):
    t = TMatrixTable()
    t.ct = ct
    return t

cdef class TMatrixTable(BaseType):
    pass
