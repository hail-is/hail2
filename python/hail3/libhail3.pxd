# distutils: language=c++
from libcpp cimport bool
from libcpp.memory cimport shared_ptr
from libcpp.string cimport string
from libc.stdint cimport uint64_t

cdef extern from "type.hh" namespace "hail":
    cdef cppclass TInt32:
        bool required
        string to_string()

cdef extern from "region.hh" namespace "hail":
    cdef cppclass TypedRegionValue:
        string to_string()

cdef extern from "context.hh" namespace "hail":
    cdef cppclass Context:
        const TInt32 *int32_type(bool required)

cdef extern from "matrixtable.hh" namespace "hail":
    cdef cppclass MatrixTable:
        MatrixTable(Context c, string filename)
        shared_ptr[MatrixTableIterator] iterator()
        uint64_t count_rows()

    cdef cppclass MatrixTableIterator:
        bool has_next()
        TypedRegionValue next()
