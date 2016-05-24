/*
**  Author: Brandon Smith
**  Repo:   https://github.com/VectorCell/physmem
*/

#include <iostream>
#include <vector>

#include <cstdio>
#include <cstdint>

using namespace std;

const char * const HEX_FORMAT = (sizeof(size_t) == 8) ? "%lx" : "%x";
const char * const DEC_FORMAT = (sizeof(size_t) == 8) ? "%lu" : "%u";

const int BUFFERSIZE = 16 * 1024;

vector<uint8_t> buf(BUFFERSIZE);

int64_t get_chunk (const size_t addr, size_t num_bytes, FILE *outfile)
{
	void *buffer = (void *)&buf[0];
	FILE *f = fopen("/dev/mem", "rb");
	if (f) {
		fseek(f, addr, SEEK_SET);
		int64_t bytes_read = 0;
		size_t count = 0;
		while ((count = fread(buffer, sizeof(uint8_t), min(num_bytes, buf.size()), f)) > 0) {
			bytes_read += count;
			num_bytes -= count;
			fwrite(buffer, sizeof(uint8_t), count, outfile);
		}
		fclose(f);
		return bytes_read;
	} else {
		cerr << "ERROR: must be run with root privileges" << endl;
		exit(1);
	}
}

int main (int argc, char *argv[])
{
	if (argc < 2) {
		cerr << endl;
		cerr << "Usage:  " << argv[0] << " ADDRESS [NUM_BYTES=4] [OUTPUT_FILE=stdout]" << endl;
		cerr << endl;
		cerr << "        ADDRESS:     memory address to be read (hex address)" << endl;
		cerr << "        NUM_BYTES:   the number of bytes to read (decimal integer)" << endl;
		cerr << "        OUTPUT_FILE: the file to dump memory contents to" << endl;
		cerr << endl;
		return EXIT_FAILURE;
	}

	size_t address;
	sscanf(argv[1], HEX_FORMAT, &address);

	size_t num_bytes = 4;
	if (argc >= 3) {
		sscanf(argv[2], DEC_FORMAT, &num_bytes);
	}

	FILE *outfile = stdout;
	if (argc >= 4) {
		outfile = fopen(argv[3], "wb");
		if (outfile == NULL) {
			cerr << "ERROR: unable to write to " << argv[3] << endl;
			return EXIT_FAILURE;
		}
	}

	int64_t bytes_read = 0;
	do {
		bytes_read = get_chunk(address, num_bytes, stdout);
		num_bytes -= bytes_read;
		address += bytes_read;
	} while (bytes_read > 0);

	fclose(outfile);

	return EXIT_SUCCESS;
}
