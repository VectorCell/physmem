# physmem

Similar to devmem2, but allows for reading multiple words.

```
Usage:  ./physmem MODE ADDRESS [-f FILE] [-n NUM_BYTES]

        MODE:      The physmem operating mode, either r or w.

        ADDRESS:   The memory address to be read/written (hex address).

        NUM_BYTES: The number of bytes to read (decimal integer).
                   For read mode, the default value is the word size
                   on the current machine. This is 4 for 32-bit machines,
                   or 8 for 64-bit machines.
                   For write mode, this defaults to the size of the
                   input file.

        FILE:      Where to read to, or write from.
                   The default in/out is stdin/stdout.
```

Designed for Linux, maybe works on BSD, and OS X with
[modification](http://apple.stackexchange.com/questions/114319/how-to-access-dev-mem-in-osx).

| Precompiled binaries               |
|------------------------------------|
| [Linux x86-64](bin/physmem-x86_64) |
| [Linux ARMv6](bin/physmem-armv6l)  |
| [Linux ARMv7](bin/physmem-armv7l)  |

