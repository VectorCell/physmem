# physmem

Similar to devmem2, but allows for reading multiple words.

```
Usage:  ./physmem ADDRESS [NUM_BYTES=4] [OUTPUT_FILE=stdout]

		ADDRESS:     memory address to be read (hex address)
		NUM_BYTES:   the number of bytes to read (decimal integer)
		OUTPUT_FILE: the file to dump memory contents to
```

Designed for Linux, maybe works on BSD, and OS X with
[modification](http://apple.stackexchange.com/questions/114319/how-to-access-dev-mem-in-osx).

| Precompiled binaries               |
|------------------------------------|
| [Linux x86-64](bin/physmem-x86_64) |
| [Linux ARMv6](bin/physmem-armv6l)  |
| [Linux ARMv7](bin/physmem-armv7l)  |

