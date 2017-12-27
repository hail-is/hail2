
# OPTDEBUGFLAGS = -O3 -DNDEBUG -march=native
# OPTDEBUGFLAGS = -O -DNDEBUG -pg
OPTDEBUGFLAGS = -g
# OPTDEBUGFLAGS = -g -pg

CC = gcc
CXX = g++

INCLUDES = -Icpp

CFLAGS = $(INCLUDES) -Wall -Werror -MD -fPIC $(OPTDEBUGFLAGS)
CXXFLAGS = $(INCLUDES) -std=c++17 -Wall -Werror -MD -fPIC $(OPTDEBUGFLAGS)

LDFLAGS = -Lcpp

LIBS = -lhail3 -lfmt -llz4 -lz

-include cpp/*.d

.PHONY: all
all: cpp/main python

#  -fno-exceptions
cpp/libhail3.a: cpp/gzstream.o cpp/region.o cpp/type.o cpp/matrixtable.o cpp/inputbuffer.o
	rm -f $@
	ar -r $@ $^

cpp/main: cpp/main.o cpp/libhail3.a
	g++ $(CXXFLAGS) $(LDFLAGS) -o $@ cpp/main.o $(LIBS)

.PHONY: python
python: cpp/libhail3.a
	cd python && CC=$(CC) CXX=$(CXX) python3 setup.py build_ext --inplace

clean:
	rm -rf cpp/libhail3.a
	rm -f cpp/*.o
	rm -f cpp/*.d
	rm -f cpp/main
	rm -f python/hail3/*.so
	rm -f python/hail3/*.cpp
	rm -rf python/hail3/__pycache__
