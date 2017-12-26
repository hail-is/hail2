
OPTDEBUGFLAGS = -O3 -DNDEBUG -march=native
# OPTDEBUGFLAGS = -O -DNDEBUG -pg
# OPTDEBUGFLAGS = -g
# OPTDEBUGFLAGS = -g -pg

#  -fno-exceptions
main: main.cc region.hh type.hh type.cc gzstream.C casting.hh
	g++ $(OPTDEBUGFLAGS) -fno-rtti -std=c++17 -Wall -Werror -I/home/cotton/opt/fmt-3193460/include -I. main.cc type.cc region.cc gzstream.C -o main -L/home/cotton/opt/fmt-3193460/lib -llz4 -lz -lfmt

clean:
	rm -f main *.o
