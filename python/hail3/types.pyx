# distutils: language=c++
from libcpp cimport bool
from libcpp.memory cimport shared_ptr, make_shared
from libcpp.string cimport string

cdef extern from "type.hh": # namespace "hail":
    cdef cppclass CTInt32 "TInt32":
        bool required
        string to_string()

cdef class TInt32:
    cdef shared_ptr[CTInt32] t

    def __cinit__(self, bool required):
        self.t = make_shared[CTInt32](required)

    @property
    def required(self):
        return self.t.get().required

    def __repr__(self):
        return self.t.get().to_string().decode('ascii')
