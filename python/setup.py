from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

extensions = [
    Extension("hail3.types", ["hail3/types.pyx"],
              language='c++',
              include_dirs = ['../cpp'],
              libraries = ['hail3', 'fmt', 'lz4', 'z'],
              library_dirs = ['../cpp'])
]

setup(ext_modules=cythonize(extensions))
