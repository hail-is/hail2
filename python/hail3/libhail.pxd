# distutils: language=c++
# cython: language_level=3
from libcpp cimport bool
from libcpp.memory cimport shared_ptr
from libcpp.string cimport string
from libcpp.vector cimport vector
from libc.stdint cimport uint64_t

cdef extern from "type.hh" namespace "hail":
    cdef cppclass BaseTypeKind "hail::BaseType::Kind":
        pass

    cdef cppclass BaseType:
        BaseTypeKind kind
        string to_string()

    cdef cppclass TMatrixTable(BaseType):
        pass

    cdef cppclass Type(BaseType):
        bool required
        uint64_t alignment
        uint64_t size
        const Type *fundamental_type
        bool is_fundamental()

    cdef cppclass TBoolean(Type):
        pass

    cdef cppclass TInt32(Type):
        pass

    cdef cppclass TInt64(Type):
        pass

    cdef cppclass TFloat32(Type):
        pass

    cdef cppclass TFloat64(Type):
        pass

    cdef cppclass TString(Type):
        pass

    cdef cppclass Field:
        string name
        const Type *type

    cdef cppclass TStruct(Type):
        vector[Field] fields

    cdef cppclass TArray(Type):
        const Type *element_type

    cdef cppclass TComplex(Type):
        const Type *representation

    cdef cppclass TSet(TComplex):
        const Type *element_type

    cdef cppclass TCall(TComplex):
        pass

    cdef cppclass TLocus(TComplex):
        string gr

    cdef cppclass TAltAllele(TComplex):
        pass

    cdef cppclass TVariant(TComplex):
        string gr

cdef extern from "type.hh" namespace "hail::BaseType":
    cdef BaseTypeKind MATRIXTABLE
    cdef BaseTypeKind TABLE
    cdef BaseTypeKind MATRIX
    cdef BaseTypeKind BOOLEAN
    cdef BaseTypeKind INT32
    cdef BaseTypeKind INT64
    cdef BaseTypeKind FLOAT32
    cdef BaseTypeKind FLOAT64
    cdef BaseTypeKind STRING
    cdef BaseTypeKind STRUCT
    cdef BaseTypeKind ARRAY
    cdef BaseTypeKind SET
    cdef BaseTypeKind CALL
    cdef BaseTypeKind LOCUS
    cdef BaseTypeKind ALTALLELE
    cdef BaseTypeKind VARIANT
    cdef BaseTypeKind INTERVAL

cdef extern from "region.hh" namespace "hail":
    cdef cppclass TypedRegionValue:
        string to_string()

cdef extern from "context.hh" namespace "hail":
    cdef cppclass Context:
        const TBoolean *boolean_type(bool required)
        const TInt32 *int32_type(bool required)
        const TInt64 *int64_type(bool required)
        const TFloat32 *float32_type(bool required)
        const TFloat64 *float64_type(bool required)
        const TString *string_type(bool required)
        const TCall *call_type(bool required)
        const TAltAllele *alt_allele_type(bool required)

cdef extern from "matrixtable.hh" namespace "hail":
    cdef cppclass MatrixTable:
        MatrixTable(Context c, string filename)
        shared_ptr[MatrixTableIterator] iterator()
        uint64_t count_rows()
        const TMatrixTable *typ "type"

    cdef cppclass MatrixTableIterator:
        bool has_next()
        TypedRegionValue next()
