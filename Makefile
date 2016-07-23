CSTD   := c99
CPPSTD := c++11

ifeq "$(CXX)" "g++"
	GCCVERSIONLT48 := $(shell expr `gcc -dumpversion` \< 4.8)
	ifeq "$(GCCVERSIONLT48)" "1"
		CPPSTD := c++0x
	endif
endif

CFLAGS   := -pedantic -std=$(CSTD) -Wall -Werror -O3
CPPFLAGS := -pedantic -std=$(CPPSTD) -Wall -Werror -O3
LIBFLAGS := 

all : physmem

physmem : physmem.cc
	$(CXX) $(CPPFLAGS) -o physmem physmem.cc $(LIBFLAGS)
	cp physmem bin/physmem-$(shell lscpu | grep Architecture | awk '{print $$2}')

test : all
	./physmem | cat
	sudo ./physmem r 0xd0000000 -n 256 | hexdump
	sudo ./physmem r 0xd0000000 -n 25600 | hexdump

clean :
	rm -f *.d
	rm -f *.o
	rm -f physmem

-include *.d
