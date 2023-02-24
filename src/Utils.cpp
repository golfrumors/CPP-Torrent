#include <assert.h>
#include <stdint.h>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <string>
#include <bitset>
#include <stdio.h>
#include <string.h>

//AVX intrinsic functions header
#include <immintrin.h>

#include "../include/Utils.h"

//fast memory copy util func, only available on AVX2 compliant cpus
void fastMemcpy(void* ptrDest, void* ptrSrc, size_t nBytes) {
        assert(nBytes % 32 == 0);
        assert((intptr_t(ptrDest) & 31) == 0);
        assert((intptr_t(ptrSrc) & 31) == 0);
        const __m256i *pSrc = reinterpret_cast<const __m256i*>(ptrSrc);
        __m256i *pDest = reinterpret_cast<__m256i*>(ptrDest);
        int64_t nVects = nBytes / sizeof(*pSrc);
        for (; nVects > 0; nVects--, pSrc++, pDest++) {
            const __m256i loaded = _mm256_stream_load_si256(pSrc);
            _mm256_stream_si256(pDest, loaded);
        }
        _mm_sfence();
}

std::string urlEncode(const std::string& value) {
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;
	for(char c : value) {
		if(isalnum(c) == '-' || c == '_' || c == '.' || c == '~'){
			escaped << c;
			continue;
		}

		escaped << std::uppercase;
		escaped << '%' << std::setw(2) << int((unsigned char) c);
		escaped << std::nouppercase;
	}

	return escaped.str();
}

std::string hexDecode(const std::string& value) {
	int hashLength = value.length();
	std::string decoded;
	for(int i = 0; i < hashLength; i += 2) {
		std::string byte = value.substr(i, 2);
		char chr = (char) (int)strtol(byte.c_str(), nullptr, 16);
		decoded.push_back(chr);
	}

	return decoded;
}

std::string hexEncode(const std::string& value) {
	static const char hexDig[] = "0123456789ABCDEF";

	std::string encoded;
	encoded.reserve(value.length() * 2);

	for(unsigned char c : value) {
		encoded.push_back('\\');
		encoded.push_back('x');
		encoded.push_back(hexDig[c >> 4]);
		encoded.push_back(hexDig[c & 15]);
	}

	return encoded;
}

bool hasPiece(const std::string& bitfield, int pieceIndex) {
	int byteIndex = floor(pieceIndex / 8);
	int bitIndex = pieceIndex % 8;

	return (bitfield[byteIndex] >> (7- bitIndex) & 1) != 0;
}

void setPiece(std::string& bitfield, int pieceIndex) {
	int byteIndex = floor(pieceIndex / 8);
	int bitIndex = pieceIndex % 8;

	bitfield[byteIndex] |= 1 << (7 - bitIndex);
}

//bytesToInteger from string using bitwise operation to convert
int bytesToInt(std::string bytes) {
	std::string binStr;
    long byteCount = bytes.size();
    for (int i = 0; i < byteCount; i++)
        binStr += std::bitset<8>(bytes[i]).to_string();
    return stoi(binStr, 0, 2);
}

//fast string copy
void fastStrCopy(char* buffer, std::size_t buffersize, const std::string& str) {
	std::size_t len = std::min(buffersize - 1, str.size());
	memcpy(buffer, &str[0], len);
	buffer[len] = '\0';
}

std::string formatTime(long seconds) {
	if (seconds < 0)
		return "inf";

	std::string result;
    std::string h = std::to_string(seconds / 3600);
    std::string m = std::to_string((seconds % 3600) / 60);
    std::string s = std::to_string(seconds % 60);
    std::string hh = std::string(2 - h.length(), '0') + h;
    std::string mm = std::string(2 - m.length(), '0') + m;
    std::string ss = std::string(2 - s.length(), '0') + s;
    if (hh.compare("00") != 0) {
        result = hh + ':' + mm + ":" + ss;
    }
    else {
        result =  mm + ":" + ss;
    }
    return result;
}