CSTD   := c11
CPPSTD := c++11

CFLAGS   := -pedantic -std=$(CSTD) -Wall -Werror -O3
CPPFLAGS := -pedantic -std=$(CPPSTD) -Wall -Werror -O3
LIBFLAGS := 

all : physmem

physmem : physmem.cc
	$(CXX) $(CPPFLAGS) -o physmem physmem.cc $(LIBFLAGS)

test : all
	./physmem 0x00000000

clean :
	rm -f *.d
	rm -f *.o
	rm -f physmem

-include *.d
