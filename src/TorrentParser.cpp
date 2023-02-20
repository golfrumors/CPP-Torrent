#include <regex>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <bencode/BItem.h>
#include <bencode/Decoder.h>
#include <bencode/bencoding.h>
#include <loguru/loguru.hpp>

#include "include/Sha.h"
#include "include/TorrentParser.h"

#define HASH_LENGTH 20

TorrentParser::TorrentParser(const std::string& filePth) {
	LOG_F(INFO, "Parsing file %s...", filePth.c_str());
	std::ifstream fileStream(filePt, std::ifstream::binary);
	std::shared_ptr<bencoding::BItem> decodedTorFile = bencoding::decode(fileStream);
	std::shared_ptr<bencoding::BDictionary> rootDiict = std::dynamic_pointer_cast<bencoding::BDictionary>(decodedTorFile);

	root = rootDict;

	LOG_F(INFO, "Parsing file %s... Done", filePth.c_str());
}

std::shared_ptr<bencoding::BItem> TorrentParser::get(std::string key) const {
	std::shared_ptr<bencoding::BItem> value = root->getValue(key);
	return value;
}

std::string TorrentParser::getInfoHash() const {
	std::shared_ptr<bencoding::BItem> infDict = get("info");
	std::string infString = bencoding::encode(infDict);
	std::string shaHash = sha(infString);
	return shaHash;
}

std::vector<std::string> TorrentParser::splitPieceHashes() const {
	std::shared_ptr<bencoding::BItem> piecesVal = get("pieces");

	if(!piecesVal)
		throw std::runtime_error("No pieces key in torrent file");

	std::string piecesStr = std::dynamic_pointer_cast<bencoding::BString>(piecesVal)->getValue();

	std::vector<std::string> pieceHashes;

	assert(piecesStr.size() % HASH_LENGTH == 0);
	int numPieces = (int) piecesStr.size() / HASH_LENGTH;
	pieceHashes.reserve(numPieces);

	for(int i = 0; i < numPieces; i++) {
		pieceHashes.push_back(pieces.substr(i * HASH_LENGTH, HASH_LENGTH));
	}

	return pieceHashes;
}

long TorrentParser::getFileSize() const {
	std::shared_ptr<bencoding::BItem> fileSizeItem = get("length");
	if(!fileSizeItem)
		throw std::runtime_error("No length key in torrent file");

	long fileSize = std::dynamic_pointer_cast<bencoding::BInteger>(fileSizeItem)->value();
	return fileSize;
}

long TorrentParser::getPieceLength() const {
	std::shared_ptr<bencoding::BItem> pieceLengthItem = get("piece length");
	if(!pieceLengthItem)
		throw std::runtime_error("No piece length key in torrent file");

	long pieceLength = std::dynamic_pointer_cast<bencoding::BInteger>(pieceLengthItem)->value();
	return pieceLength;
}

std::string TorrentParser::getFileName() const {
	std::shared_ptr<bencoding::BItem> fileNameItem = get("name");
	if(!fileNameItem)
		throw std::runtime_error("No name key in torrent file");

	std::string fileName = std::dynamic_pointer_cast<bencoding::BString>(fileNameItem)->getValue();
	return fileName;
}

std::string TorrentParser::getAnnounce() const {
	std::shared_ptr<bencoding::BItem> announceItem = get("announce");
	if(!announceItem)
		throw std::runtime_error("No announce key in torrent file");
	std::string announce = std::dynamic_pointer_cast<bencoding::BString>(announceItem)->getValue();
	return announce;
}
