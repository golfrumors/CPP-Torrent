#ifndef SHA_H
#define SHA_H

#include <string>
#include <iostream>

class SHA {
	public:
		SHA();
		void update(const std::string& string);
		void update(std::istream& is);
		std::string final();
		static std::string fromFile(const std::string& filename);

	private:
		typedef unsigned long int uint32;
		typedef unsigned long long int uint64;

		static const unsigned int DIGEST_INTS = 5;
		static const unsigned int BLOCK_INTS = 16;
		static const unsigned int BLOCK_BYTES = BLOCK_INTS * 4;

		uint32 digest[DIGEST_INTS];
		std::string buffer;
		uint64 transforms;

		void reset();
		void transform(uint32 block[BLOCK_BYTES]);

		static void bufferToBlock(const std::string &buffer, uint32 block[BLOCK_BYTES]);
		static void read(std::istream &is, std::string& string, int max);
};

std::string toSha(const std::string& string);

#endif
