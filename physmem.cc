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

const char * const HEX_FORMAT = (sizeof(uintptr_t) == 8) ? "%lx" : "%x";
const char * const DEC_FORMAT = (sizeof(uintptr_t) == 8) ? "%lu" : "%u";
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

// for testing
//uintptr_t vtop (uintptr_t vaddr)
//{
//	FILE *pagemap;
//	intptr_t paddr = 0;
//	int offset = (vaddr / sysconf(_SC_PAGESIZE)) * sizeof(uint64_t);
//	uint64_t e;
//
//	// https://www.kernel.org/doc/Documentation/vm/pagemap.txt
//	if ((pagemap = fopen("/proc/self/pagemap", "rb"))) {
//		if (lseek(fileno(pagemap), offset, SEEK_SET) == offset) {
//			if (fread(&e, sizeof(uint64_t), 1, pagemap)) {
//				if (e & (1ULL << 63)) { // page present ?
//					paddr = e & ((1ULL << 54) - 1); // pfn mask
//					paddr = paddr * sysconf(_SC_PAGESIZE);
//					// add offset within page
//					paddr = paddr | (vaddr & (sysconf(_SC_PAGESIZE) - 1));
//				}
//			}
//		}
//		fclose(pagemap);
//	} else {
//		cerr << "ERROR: unable to open pagemap" << endl;
//	}
//	return paddr;
//}

uintptr_t vtop (const uintptr_t addr) {

	#define PAGEMAP_LENGTH 8
	#define PAGE_SHIFT 12

	// Open the pagemap file for the current process
	FILE *pagemap = fopen("/proc/self/pagemap", "rb");

	if (pagemap == NULL) {
		cerr << "ERROR: unable to open pagemap" << endl;
		return 0;
	}

	// Seek to the page that the buffer is on
	uintptr_t offset = addr / getpagesize() * PAGEMAP_LENGTH;
	if (fseek(pagemap, offset, SEEK_SET) != 0) {
		fprintf(stderr, "Failed to seek pagemap to proper location\n");
		return 0;
	}

	// The page frame number is in bits 0-54 so read the first 7 bytes and clear the 55th bit
	uintptr_t page_frame_number = 0;
	size_t count = fread(&page_frame_number, 1, PAGEMAP_LENGTH - 1, pagemap);
	if (count != PAGEMAP_LENGTH - 1) {
		cerr << "unable to read requested number of bytes from pagemap" << endl;
		return 0;
	}

	page_frame_number &= 0x7FFFFFFFFFFFFF;

	fclose(pagemap);

	return (page_frame_number << PAGE_SHIFT) + (addr % (1 << PAGE_SHIFT));
}

uint32_t read_paddr_32 (const uintptr_t paddr)
{
	uint32_t val32 = 0;
	FILE *f = fopen("/dev/mem", "rb");
	if (f) {
		size_t count = fread(&val32, 1, sizeof(uint32_t), f);
		fseek(f, paddr, SEEK_SET);
		if (count != sizeof(uint32_t)) {
			cerr << "unable to read 32 bits from /dev/mem" << endl;
		}
		return val32;
	} else {
		cerr << "ERROR: unable to open /dev/mem" << endl;
		exit(1);
	}
}

bool write_paddr_32 (const uintptr_t paddr, uint32_t data)
{
	FILE *f = fopen("/dev/mem", "wb");
	if (f) {
		size_t count = fwrite(&data, 1, sizeof(uint32_t), f);
		fseek(f, paddr, SEEK_SET);
		if (count != sizeof(uint32_t)) {
			cerr << "unable to write 32 bits to /dev/mem" << endl;
			return false;
		}
	} else {
		cerr << "ERROR: unable to open /dev/mem" << endl;
		exit(1);
	}
	return data == read_paddr_32(paddr);
}

uint64_t read_paddr_64 (const uintptr_t paddr)
{
	uint64_t val64 = 0;
	FILE *f = fopen("/dev/mem", "rb");
	if (f) {
		size_t count = fread(&val64, 1, sizeof(uint64_t), f);
		fseek(f, paddr, SEEK_SET);
		if (count != sizeof(uint64_t)) {
			cerr << "unable to read 64 bits from /dev/mem" << endl;
		}
		return val64;
	} else {
		cerr << "ERROR: unable to open /dev/mem" << endl;
		exit(1);
	}
}

bool write_paddr_64 (const uintptr_t paddr, uint64_t data)
{
	FILE *f = fopen("/dev/mem", "wb");
	if (f) {
		size_t count = fwrite(&data, 1, sizeof(uint64_t), f);
		fseek(f, paddr, SEEK_SET);
		if (count != sizeof(uint64_t)) {
			cerr << "unable to write 64 bits to /dev/mem" << endl;
			return false;
		}
	} else {
		cerr << "ERROR: unable to open /dev/mem" << endl;
		exit(1);
	}
	return data == read_paddr_64(paddr);
}

//#pragma GCC push_options
//#pragma GCC optimize ("O0")
bool do_test ()
{
	uint8_t pad_front[64];
	const uint32_t val32 = 0xdeadbeef;
	const uint64_t val64 = 0xcafed00dcafebabe;
	uint8_t pad_back[64];

	for (int k = 0; k < 64; ++k) {
		pad_front[k] = pad_back[k] = 0;
	}

	uintptr_t ptr32  = reinterpret_cast<uintptr_t>(&val32);
	uintptr_t ptr64  = reinterpret_cast<uintptr_t>(&val64);

	uintptr_t ptr32_paddr = vtop(ptr32);
	uintptr_t ptr64_paddr = vtop(ptr64);

	printf("value of val32: %x\n", val32);
	printf("value of val64: %lx\n", val64);
	printf("virtual address of val32: %p\n", (void*)ptr32);
	printf("virtual address of val64: %p\n", (void*)ptr64);
	printf("physical address of val32: %p\n", (void*)ptr32_paddr);
	printf("physical address of val64: %p\n", (void*)ptr64_paddr);
	printf("value at vaddr %p: %x\n", (void*)ptr32, *(uint32_t*)ptr32);
	printf("value at vaddr %p: %lx\n", (void*)ptr64, *(uint64_t*)ptr64);
	printf("value at paddr %p: %x\n", (void*)ptr32_paddr, read_paddr_32(ptr32_paddr));
	printf("value at paddr %p: %lx\n", (void*)ptr64_paddr, read_paddr_64(ptr64_paddr));

	if (val32 != read_paddr_32(ptr32_paddr)) {
		return false;
	}
	if (val64 != read_paddr_64(ptr64_paddr)) {
		return false;
	}

	for (int k = 0; k < 64; ++k) {
		if (pad_front[k] != 0) {
			return false;
		}
		if (pad_back[k] != 0) {
			return false;
		}
	}

	return true;
}
//#pragma GCC pop_options

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
		sscanf(addr_str, HEX_FORMAT, &args.address);
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
		sscanf(n_bytes_str, DEC_FORMAT, &args.num_bytes);
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

//	fprintf(stderr, "args.mode:      %s\n", (args.mode == MODE_READ) ? "read" : "write");
//	fprintf(stderr, "args.address:   %p\n", (void*)args.address);
//	fprintf(stderr, "args.num_bytes: %zd\n", args.num_bytes);
//	fprintf(stderr, "args.filename:  %s\n", args.filename);
//	fprintf(stderr, "args.file:      %p\n", (void*)args.file);

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
		bool passed = do_test();
		if (passed) {
			cerr << "PASS" << endl;
		} else {
			cerr << "FAIL" << endl;
		}
	}

	fclose(args.file);

	return EXIT_SUCCESS;
}
