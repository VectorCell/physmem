

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