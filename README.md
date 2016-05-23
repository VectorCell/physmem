# physmem

Similar to devmem2 but allows for reading multiple words.

```
Usage:  ./physmem ADDRESS [NUM_BYTES=4] [OUTPUT_FILE=stdout]

		ADDRESS:     memory address to being read (hex address)
		NUM_BYTES:   the number of bytes to read (decimal integer)
		OUTPUT_FILE: the file to dump memory contents to
```

| Precompiled binaries         |
|------------------------------|
| [x86-64](bin/physmem-x86_64) |
| [ARMv6](bin/physmem-armv6l)  |
| [ARMv7](bin/physmem-armv7l)  |

