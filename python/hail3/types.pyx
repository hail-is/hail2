# distutils: language=c++
# cython: language_level=3
from libcpp cimport bool
from libcpp.memory cimport shared_ptr, make_shared
from libcpp.string cimport string
from libc.stdint cimport uintptr_t, uint64_t

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

cdef region_value_to_python(libhail.TypedRegionValue ctrv):
    cdef libhail.BaseTypeKind k = ctrv.typ.kind
    cdef uint64_t n
    cdef uint64_t i
    cdef const libhail.TStruct *ts
    if k == libhail.BOOLEAN:
        return ctrv.load_bool()
    elif k == libhail.INT32:
        return ctrv.load_int()
    elif k == libhail.INT64:
        return ctrv.load_long()
    elif k == libhail.FLOAT32:
        return ctrv.load_float()
    elif k == libhail.FLOAT64:
        return ctrv.load_double()
    elif k == libhail.STRING:
        return ctrv.load_string().decode('ascii')
    elif k == libhail.ARRAY:
        n = ctrv.array_size()
        i = 0
        a = []
        while i < n:
            if (ctrv.is_element_defined(i)):
                a.append(region_value_to_python(ctrv.load_element(i)))
            else:
                a.append(None)
            i = i + 1
        return a
    elif k == libhail.STRUCT:
        ts = <const libhail.TStruct *>ctrv.typ
        n = ts.fields.size()
        i = 0
        s = {}
        while i < n:
            if ctrv.is_field_defined(i):
                s[ts.fields[i].name.decode('ascii')] = region_value_to_python(ctrv.load_field(i))
            i = i + 1
        return s
    else:
        raise RuntimeError('unknown type kind')

cdef class MatrixTable(object):
    cdef Context context
    cdef shared_ptr[libhail.MatrixTable] mt

    def __init__(self, Context c, str filename):
        self.context = c
        self.mt = make_shared[libhail.MatrixTable](c.context[0], <string>filename.encode('ascii'))

    # FIXME leaves file open
    def rows(self):
        cdef shared_ptr[libhail.MatrixTableIterator] ci = self.mt.get().iterator()
        rs = []
        while ci.get().has_next():
            rs.append(region_value_to_python(ci.get().next()))
        return rs

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
