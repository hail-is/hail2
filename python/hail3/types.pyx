# distutils: language=c++
from libcpp cimport bool
from libcpp.memory cimport shared_ptr, make_shared
from libcpp.string cimport string

cimport libhail3

cdef class Context(object):
    cdef libhail3.Context *context

    def __cinit__(self):
        self.context = new libhail3.Context()

cdef TypedRegionValue_init(libhail3.TypedRegionValue ctrv):
    trv = TypedRegionValue()
    trv.ctrv = ctrv
    return trv

cdef class TypedRegionValue(object):
    cdef libhail3.TypedRegionValue ctrv

    def __repr__(self):
        return self.ctrv.to_string().decode('ascii')

cdef MatrixTableIterator_init(shared_ptr[libhail3.MatrixTableIterator] ci):
  i = MatrixTableIterator()
  i.ci = ci
  return i
    
cdef class MatrixTableIterator(object):
    cdef shared_ptr[libhail3.MatrixTableIterator] ci

    def __iter__(self):
        return self

    def __next__(self):
        if self.ci.get().has_next():
            return TypedRegionValue_init(self.ci.get().next())
        else:
            raise StopIteration()

cdef class MatrixTable(object):
    cdef shared_ptr[libhail3.MatrixTable] mt

    def __init__(self, Context c, str filename):
        self.mt = make_shared[libhail3.MatrixTable](c.context[0], <string>filename.encode('ascii'))

    # FIXME leaves file open
    def rows(self):
        return MatrixTableIterator_init(self.mt.get().iterator())

    def count_rows(self):
        return self.mt.get().count_rows()

cdef class TInt32:
    cdef const libhail3.TInt32 *t

    def __cinit__(self, Context c, bool required):
        self.t = c.context.int32_type(required)

    @property
    def required(self):
        return self.t.required

    def __repr__(self):
        return self.t.to_string().decode('ascii')
