
OPTDEBUGFLAGS = -O3 -DNDEBUG -march=native
# OPTDEBUGFLAGS = -O3 -DNDEBUG -march=native -pg
# OPTDEBUGFLAGS = -g
# OPTDEBUGFLAGS = -g -gp

main: main.cc region.hh
	g++ $(OPTDEBUGFLAGS) -std=c++1z -Wall -Werror main.cc -o main -llz4

clean:
	rm -f main *.o
