# distutils: language=c++
# cython: language_level=3
from libcpp cimport bool
from libcpp.memory cimport shared_ptr, make_shared
from libcpp.string cimport string

from hail3 cimport libhail

cdef class Context(object):
    cdef libhail.Context *context

    def __cinit__(self):
        self.context = new libhail.Context()

        print('''Welcome to
      __  __      <>_______
     / / / /___  __/ /__  /
    / /_/ / __ `/ / / /_ < 
   / __  / /_/ / / /___/ / 
  /_/ /_/\__,_/_/_//____/   version TODO''')

    def read(self, filename):
      return MatrixTable(self, filename)

    def boolean_type(self, bool required):
      return Type_init(self.context.boolean_type(required))

    def int32_type(self, bool required):
      return Type_init(self.context.int32_type(required))

    def int64_type(self, bool required):
      return Type_init(self.context.int64_type(required))

    def float32_type(self, bool required):
      return Type_init(self.context.float32_type(required))

    def float64_type(self, bool required):
      return Type_init(self.context.float64_type(required))

    def string_type(self, bool required):
      return Type_init(self.context.string_type(required))

    def call_type(self, bool required):
      return Type_init(self.context.call_type(required))

    def alt_allele_type(self, bool required):
      return Type_init(self.context.alt_allele_type(required))

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
    cdef shared_ptr[libhail.MatrixTable] mt
    cdef TMatrixTable _typ

    def __init__(self, Context c, str filename):
        self.mt = make_shared[libhail.MatrixTable](c.context[0], <string>filename.encode('ascii'))
        self._typ = TMatrixTable_init(self.mt.get().typ)

    # FIXME leaves file open
    def rows(self):
        return MatrixTableIterator_init(self.mt.get().iterator())

    def count_rows(self):
        return self.mt.get().count_rows()

    @property
    def typ(self):
        return self._typ

cdef class BaseType:
    pass

cdef Type_init(const libhail.Type *ct):
    t = Type()
    t.ct = ct
    return t

cdef class Type(BaseType):
    cdef const libhail.Type *ct

    @property
    def required(self):
        return self.ct.required

    def __repr__(self):
        return self.ct.to_string().decode('ascii')

cdef TMatrixTable_init(const libhail.TMatrixTable *ct):
    t = TMatrixTable()
    t.ct = ct
    return t

cdef class TMatrixTable(BaseType):
    cdef const libhail.TMatrixTable *ct

    def __repr__(self):
        return self.ct.to_string().decode('ascii')
