CSTD   := c11
CPPSTD := c++11

CFLAGS   := -pedantic -std=$(CSTD) -Wall -Werror -O3
CPPFLAGS := -pedantic -std=$(CPPSTD) -Wall -Werror -O3
LIBFLAGS := 

all : physmem

physmem : physmem.cc
	$(CXX) $(CPPFLAGS) -o physmem physmem.cc $(LIBFLAGS)
	cp physmem bin/physmem-$(shell lscpu | grep Architecture | awk '{print $$2}')

test : all
	./physmem | cat
	sudo ./physmem 0xd0000000 256 | hexdump

clean :
	rm -f *.d
	rm -f *.o
	rm -f physmem

-include *.d
