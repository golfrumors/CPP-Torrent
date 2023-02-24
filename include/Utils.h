#ifndef UTILS_H
#define UTILS_H

#include <string>

std::string urlEncode(const std::string& value);

std::string hexDecode(const std::string& hex);

std::string hexEncode(const std::string& value);

bool hasPiece(const std::string& bitfield, int pieceIndex);

void setPiece(std::string& bitfield, int pieceIndex);

int bytesToInt(std::string bytes);

std::string formatTime(long seconds);

void fastMemcpy(void* ptrDest, void* ptrSrc, size_t nBytes);

void fastStrCopy(char* buffer, std::size_t buffersize, const std::string& str);
#endif // UTILS_H