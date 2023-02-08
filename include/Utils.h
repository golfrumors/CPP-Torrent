#ifndef UTILS_H
#define UTILS_H

std::string urlEncode(const std::string& value);

std::string hexDecode(const std::string& hex);

std::string hexEncode(const std::string& value);

bool hasPiece(const std::string& bitfield, int pieceIndex);

void setPiece(std::string& bitfield, int pieceIndex);

int bytesToInteger(const std::string& bytes);

std::string formatTime(long seconds);

#endif // UTILS_H