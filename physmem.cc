#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include <cstdio>
#include <cstdint>

using namespace std;

vector<uint8_t> buf(16 * 1024);

size_t dec_to_int (string str)
{
	size_t x;
	std::stringstream ss;
	ss << << str;
	ss >> x;
	return x;
}

size_t hex_to_int (string str)
{
	size_t x;
	std::stringstream ss;
	ss << std::hex << str;
	ss >> x;
	return x;
}

int64_t output_chunk (void *addr, size_t num_bytes, FILE *outfile)
{
	FILE *f = fopen("/dev/mem", "rb");
	if (f) {
		int64_t bytes_read = 0;
		vector<uint8_t> buf(32 * 1024);
		size_t count = 0;
		while ((count = fread(&buf[0], sizeof(uint8_t), buf.size(), f)) > 0) {
			bytes_read += count;
			fwrite(&buf[0], sizeof(uint8_t), count, outfile);
		}
		fclose(f);
		return bytes_read;
	} else {
		return -1;
	}
}

int main (int argc, char *argv[])
{
	if (argc < 2) {
		cerr << "Usage: physmem ADDRESS [NUM_BYTES=4] [OUTPUT_FILE=stdout]" << endl;
		return EXIT_FAILURE;
	}

	string addr_str(argv[1]);
	size_t address = hex_to_int(addr_str);
	cerr << "address: " << std::hex << address << endl;

	size_t num_bytes = 4;
	if (argc >= 3) {
		string num_bytes_str(argv[2]);
		num_bytes = dec_to_int(num_bytes_str);
	}

	FILE *outfile = stdout;
	if (argc >= 4) {
		outfile = fopen(argv[3], "wb");
		if (outfile == NULL) {
			cerr << "ERROR: unable to open " << argv[3] << endl;
			return EXIT_FAILURE;
		}
	}
	
	int64_t bytes_read = 0;
	do {
		bytes_read = output_chunk((void*)address, stdout);
		//fprintf(stderr, "read %ld bytes at address %lx\n", bytes_read, address);
		address += bytes_read;
	} while (bytes_read > 0);
	return EXIT_SUCCESS;
}
