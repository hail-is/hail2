
# OPTDEBUGFLAGS = -O3 -DNDEBUG -march=native
# OPTDEBUGFLAGS = -O3 -DNDEBUG -march=native -pg
OPTDEBUGFLAGS = -g
# OPTDEBUGFLAGS = -g -gp

main: main.cc region.hh type.hh type.cc
	g++ $(OPTDEBUGFLAGS) -std=c++1z -Wall -Werror main.cc type.cc -o main -llz4

clean:
	rm -f main *.o
