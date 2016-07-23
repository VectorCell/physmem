/*
**  Author: Brandon Smith
**  Repo:   https://github.com/VectorCell/physmem
*/

#include <iostream>
#include <vector>

#include <cstdio>
#include <cstdint>
#include <cstring>

#include <unistd.h>

using namespace std;

const char * const DASH = "-";

const size_t BUFFERSIZE = 16 * 1024;
uint8_t buf[BUFFERSIZE];

enum Mode {
	MODE_READ,
	MODE_WRITE,
	MODE_TEST
};

typedef struct args_struct {
	Mode mode;
	uintptr_t address;
	size_t num_bytes;
	char *filename;
	FILE *file;
} args_type;

size_t write_chunk (const uintptr_t addr, size_t n_bytes, FILE *infile)
{
	void *buffer = reinterpret_cast<void*>(&buf[0]);
	FILE *f = fopen("/dev/mem", "wb");
	if (f) {
		fseek(f, addr, SEEK_SET);
		size_t bytes_written = 0;
		size_t count = 0;
		while ((count = fread(buffer, sizeof(uint8_t), min(n_bytes, BUFFERSIZE), infile)) > 0) {
			bytes_written += count;
			n_bytes -= count;
			fwrite(buffer, sizeof(uint8_t), count, f);
		}
		fclose(f);
		return bytes_written;
	} else {
		cerr << "ERROR: must be run with root privileges" << endl;
		cerr << "(unable to write to /dev/mem)" << endl;
		exit(1);	
	}
}

size_t do_write (uintptr_t addr, size_t n_bytes, FILE *infile)
{
	const size_t addr_start = addr;
	size_t bytes_written = 0;
	do {
		bytes_written = write_chunk(addr, n_bytes, infile);
		n_bytes -= bytes_written;
		addr += bytes_written;
	} while (bytes_written > 0);
	return addr - addr_start;
}

// returns the number of bytes actually read this pass
size_t read_chunk (const uintptr_t addr, size_t n_bytes, FILE *outfile)
{
	void *buffer = reinterpret_cast<void*>(&buf[0]);
	FILE *f = fopen("/dev/mem", "rb");
	if (f) {
		fseek(f, addr, SEEK_SET);
		size_t bytes_read = 0;
		size_t count = 0;
		while ((count = fread(buffer, sizeof(uint8_t), min(n_bytes, BUFFERSIZE), f)) > 0) {
			bytes_read += count;
			n_bytes -= count;
			fwrite(buffer, sizeof(uint8_t), count, outfile);
		}
		fclose(f);
		return bytes_read;
	} else {
		cerr << "ERROR: must be run with root privileges" << endl;
		cerr << "(unable to read from /dev/mem)" << endl;
		exit(1);
	}
}

// returns the total number of bytes read
size_t do_read (uintptr_t addr, size_t n_bytes, FILE *outfile)
{
	const size_t addr_start = addr;
	size_t bytes_read = 0;
	do {
		bytes_read = read_chunk(addr, n_bytes, outfile);
		n_bytes -= bytes_read;
		addr += bytes_read;
	} while (bytes_read > 0);
	return addr - addr_start;
}

args_type parse_args(int argc, char *argv[])
{
	if (argc < 2) {
		cerr << endl;
		cerr << "Usage:  " << argv[0] << " MODE ADDRESS [-f FILE] [-n NUM_BYTES]" << endl;
		cerr << endl;
		cerr << "        MODE:      The physmem operating mode, either r or w." << endl;
		cerr << endl;
		cerr << "        ADDRESS:   The memory address to be read/written (hex address)." << endl;
		cerr << endl;
		cerr << "        NUM_BYTES: The number of bytes to read (decimal integer)." << endl;
		cerr << "                   For read mode, the default value is the word size" << endl;
		cerr << "                   on the current machine. This is 4 for 32-bit machines," << endl;
		cerr << "                   or 8 for 64-bit machines." << endl;
		cerr << "                   For write mode, this defaults to the size of the" << endl;
		cerr << "                   input file." << endl;
		cerr << endl;
		cerr << "        FILE:      Where to read to, or write from." << endl;
		cerr << "                   The default in/out is stdin/stdout." << endl;
		cerr << endl;
		exit(0);
	}

	char *mode_str = NULL;
	char *addr_str = NULL;
	char *filename = NULL;
	char *n_bytes_str = NULL;

	int c;
	while ((c = getopt(argc, argv, "f:n:")) != -1) {
		switch (c) {
			case 'f':
				filename = optarg;
				printf("filename: %s\n", filename);
				break;
			case 'n':
				n_bytes_str = optarg;
				printf("n_bytes_str: %s\n", n_bytes_str);
				break;
			default:
				fprintf(stderr, "unknown arg: %c\n", c);
				exit(1);
		}
	}
	for (int k = optind; k < argc; ++k) {
		if (k == optind) {
			mode_str = argv[k];
		} else if (k == optind + 1) {
			addr_str = argv[k];
		} else {
			cerr << "ERROR: too many arguments" << endl;
			exit(1);
		}
	}

	args_type args;

	// interpret mode
	if (mode_str == NULL) {
		cerr << "ERROR: must specify mode" << endl;
		exit(1);
	}
	if (mode_str[0] == 'r') {
		args.mode = MODE_READ;
	} else if (mode_str[0] == 'w') {
		args.mode = MODE_WRITE;
	} else if (mode_str[0] == 't') {
		args.mode = MODE_TEST;
	} else {
		cerr << "ERROR: unknown mode: " << mode_str << endl;
	}

	// interpret address
	if (args.mode != MODE_TEST) {
		if (addr_str == NULL) {
			cerr << "ERROR: must specify address" << endl;
			exit(1);
		}
		if (addr_str[0] == '0' && addr_str[1] == 'x') {
			addr_str += 2;
		}
		if (sizeof(uintptr_t) == sizeof(unsigned int)) {
			sscanf(addr_str, "%x", (unsigned int *)&args.address);
		} else if (sizeof(uintptr_t) == sizeof(unsigned long)) {
			sscanf(addr_str, "%lx", (unsigned long *)&args.address);
		} else {
			cerr << "This is strange" << endl;
			cerr << "A uintptr_t is " << sizeof(uintptr_t) << " bytes." << endl;
			cerr << "A int is " << sizeof(int) << " bytes." << endl;
			cerr << "A long is " << sizeof(long) << " bytes." << endl;
		}
	}

	// interpret filename
	if (filename == NULL) {
		args.filename = const_cast<char*>(DASH);
		args.file = (args.mode == MODE_READ) ? stdout : stdin;
	} else {
		if (args.mode == MODE_READ) {
			args.filename = filename;
			args.file = fopen(filename, "wb");
			if (args.file == NULL) {
				cerr << "ERROR: unable to write to " << filename << endl;
				exit(1);
			}
		} else {
			args.filename = filename;
			args.file = fopen(filename, "rb");
			if (args.file == NULL) {
				cerr << "ERROR: unable to read from " << filename << endl;
				exit(1);
			}
		}
	}

	// interpret num_bytes
	if (n_bytes_str == NULL) {
		if (args.mode == MODE_READ) {
			args.num_bytes = sizeof(uintptr_t);
		} else {
			args.num_bytes = 0;

		}
	} else {
		if (sizeof(uintptr_t) == sizeof(unsigned int)) {
			sscanf(n_bytes_str, "%u", (unsigned int *)&args.num_bytes);
		} else if (sizeof(uintptr_t) == sizeof(unsigned long)) {
			sscanf(n_bytes_str, "%lu", (unsigned long *)&args.num_bytes);
		} else {
			cerr << "This is strange" << endl;
			cerr << "A uintptr_t is " << sizeof(uintptr_t) << " bytes." << endl;
			cerr << "A int is " << sizeof(int) << " bytes." << endl;
			cerr << "A long is " << sizeof(long) << " bytes." << endl;
		}
		if (args.num_bytes == 0) {
			cout << "ERROR: can't write 0 bytes, what does that mean?" << endl;
			exit(1);
		}
	}

	return args;
}

int main (int argc, char *argv[])
{
	args_type args = parse_args(argc, argv);

	if (args.mode == MODE_READ) {
		size_t bytes_read = do_read(args.address, args.num_bytes, args.file);
		if (bytes_read != args.num_bytes) {
			cerr << "ERROR: unable to read the requested number of bytes" << endl;
			return EXIT_FAILURE;
		}
	} else if (args.mode == MODE_WRITE) {
		size_t bytes_written = do_write(args.address, args.num_bytes, args.file);
		if (args.num_bytes != 0 && bytes_written != args.num_bytes) {
			cerr << "ERROR: unable to write the requested number of bytes" << endl;
		}
	} else if (args.mode == MODE_TEST) {
//		bool passed = do_test();
//		if (passed) {
//			cerr << "PASS" << endl;
//		} else {
//			cerr << "FAIL" << endl;
//		}
	}

	fclose(args.file);

	return EXIT_SUCCESS;
}
