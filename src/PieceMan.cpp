#include <cmath>
#include <climits>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <loguru/loguru.hpp>
#include <bencode/bencoding.h>
#include <iomanip>
#include <unistd.h>
#include <sys/socket.h>

#include "../include/PieceMan.h"
#include "../include/Block.h"
#include "../include/Utils.h"

#define BLOCK_SZ 16384
#define PENDING_TIME 5
#define PROGRESS_WIDTH 40
#define PROGRESS_INTERVAL 1

PieceManager::PieceManager(
		const TorrentParser& fileParse,
		const std::string& dwnPath,
		const int maxConn
): _fileParser(fileParse), _maxConn(maxConn), _pieceLen(fileParse.getPieceLen())
{
	_missingPieces = initiatePieces();

	_downloadedFile.open(dwnPath, std::ios::binary | std::ios::out);
	_downloadedFile.seekp(fileParse.getFileSize() - 1);
	_downloadedFile.write("", 1);

	_startTime = std::time(nullptr);
	std::thread progressThread([this] {this->trackProgress();});

	progressThread.detach();
}

PieceManager::~PieceManager() {
	for (Piece* piece : _missingPieces)
		delete piece;

	for (Piece* piece : _ongoingPieces)
		delete piece;

	for (PendingReq* req : _pendingReqs)
		delete req;

	_downloadedFile.close();
}

std::vector<Piece*> PieceManager::initiatePieces() {
	std::vector<std::string> pieceHashes = _fileParser.splitPieceHashes();
	_totalPieces = pieceHashes.size();
	std::vector<Piece*> torPieces;
	_missingPieces.reserve(_totalPieces);

	long totalLen = _fileParser.getFileSize();

	int blockCount = ceil(_pieceLen / BLOCK_SZ);
	long remLen = _pieceLen;

	for(int i = 0; i < _totalPieces; i++) {
		if(i == _totalPieces - 1) {
			remLen = totalLen % _pieceLen;
			blockCount = std::max((int) ceil(remLen / BLOCK_SZ), 1);
		}

		std::vector<Block*> blocks;
		blocks.reserve(blockCount);

		for(int off = 0; off < blockCount; off++) {
			auto block = new Block;
			block->piece = i;
			block->status = missing;
			block->offset = off * BLOCK_SZ;
			
			int blockSz = BLOCK_SZ;

			if (i == _totalPieces - 1 && off == blockCount - 1)
				blockSz = remLen % BLOCK_SZ;

			block->length = blockSz;
			blocks.push_back(block);
		}

		auto piece = new Piece(i, blocks, pieceHashes[i]);
		torPieces.push_back(piece);

	}

	return torPieces;
}

bool PieceManager::complete() {
	_lock.lock();
	bool completed = _completedPieces.size() == _totalPieces;
	_lock.unlock();
	return completed;
}

void PieceManager::addPeer(const std::string& peerID, std::string bitField) {
	_lock.lock();
	_peers[peerID] = bitField;
	_lock.unlock();
	std::stringstream inf;
	inf << "Num of cons: " <<
		std::to_string(_peers.size()) << "/" << std::to_string(_maxConn);
	LOG_F(INFO, "%s",inf.str().c_str());
}

void PieceManager::updatePeer(const std::string& peerID, int ind) {
	_lock.lock();
	if(_peers.find(peerID) != _peers.end()) {
		setPiece(_peers[peerID],ind);
		_lock.unlock();
	} else { 
		_lock.unlock();
		throw std::runtime_error("Connection failed with peer " + peerID);
	}
}

void PieceManager::removePeer(const std::string& peerID) {
	if(complete())
		return;
	_lock.lock();
	auto it = _peers.find(peerID);
	if (it != _peers.end()) {
		_peers.erase(it);
		_lock.unlock();
		std::stringstream inf;
		inf << "Num of cons: " <<
			std::to_string(_peers.size()) << "/" << std::to_string(_maxConn);
		LOG_F(INFO, "%s", inf.str().c_str());
	} else {
		_lock.unlock();
		throw std::runtime_error("Trying to remove a peer: " + peerID + " that doesn't exist");
	}
}

Block* PieceManager::nextReq(std::string peerID){
	_lock.lock();
	if(_missingPieces.empty()) {
		_lock.unlock();
		return nullptr;
	}

	if(_peers.find(peerID) == _peers.end()) {
		_lock.unlock();
		return nullptr;
	}

	Block* block = expiredReq(peerID);
	if(!block) {
		block = nextOngoing(peerID);
		if(!block)
			block = getRarestPiece(peerID)->nextRequest();
	}

	_lock.unlock();

	return block;
}

Block* PieceManager::expiredReq(std::string peerID) {
	time_t currTime = std::time(nullptr);
	for(PendingReq* pending : _pendingReqs) {
		if(hasPiece(_peers[peerID], pending->block->piece)) {
			auto diff = std::difftime(currTime, pending->timeStamp);
			if(diff >= PENDING_TIME) {
				pending->timeStamp = currTime;
				LOG_F(INFO, "Block %d has expired", pending->block->offset, pending->block->piece);
				return pending->block;
			}
		}
	}
	return nullptr;
}

Block* PieceManager::nextOngoing(std::string peerID) {
	for(Piece* piece : _ongoingPieces) {
		if(hasPiece(_peers[peerID], piece->index)) {
			Block* block = piece->nextRequest();
			if(block) {
				auto currTime = std::time(nullptr);
				auto newPendingReq = new PendingReq;
				newPendingReq->block = block;
				newPendingReq->timeStamp = std::time(nullptr);
				_pendingReqs.push_back(newPendingReq);
				return block;
			}
		}
	}
	return nullptr;
}

Piece* PieceManager::getRarestPiece(std::string peerID) {
	auto comp = [](const Piece* a, const Piece* b) {return a->index < b->index; };
	std::map<Piece*, int, decltype(comp)> pieceCount(comp);

	for(Piece* piece : _missingPieces) {
		if (_peers.find(peerID) != _peers.end()) {
			if (hasPiece(_peers[peerID], piece->index))
				pieceCount[piece] += 1;
		}
	}

	Piece* rarest;
	int leastCount = INT16_MAX;
	for(auto const& [piece, count] : pieceCount) {
		if(count < leastCount) {
			leastCount = count;
			rarest = piece;
		}
	}

	_missingPieces.erase(
		std::remove(_missingPieces.begin(), _missingPieces.end(), rarest),
		_missingPieces.end()
	);

	_ongoingPieces.push_back(rarest);
	return rarest;
}

void PieceManager::blockReceived(std::string peerID, int pieceInd, int blockOff, std::string data) {
	LOG_F(INFO, "Block %d received from %s", blockOff, peerID.c_str());

	PendingReq* pendingToRem = nullptr;
	_lock.lock();
	for(PendingReq* pending : _pendingReqs) {
		if(pending->block->piece == pieceInd && pending->block->offset == blockOff) {
			pendingToRem = pending;
			break;
		}
	}

	_pendingReqs.erase(
			std::remove(_pendingReqs.begin(), _pendingReqs.end(), pendingToRem),
			_pendingReqs.end());
	
	Piece* targPiece = nullptr;
	for(Piece* piece : _ongoingPieces) {
		if(piece->index == pieceInd) {
			targPiece = piece;
			break;
		}
	}

	_lock.unlock();
	if(!targPiece)
		throw std::runtime_error("Received block for a piece that is not ongoing");

	targPiece->blockReceived(blockOff, std::move(data));

	if(targPiece->isCompleted()) {
		if(targPiece->isHashValid()) {
			write(targPiece);

			_lock.lock();
			_ongoingPieces.erase(
				std::remove(_ongoingPieces.begin(), _ongoingPieces.end(), targPiece),
				_ongoingPieces.end());

			_completedPieces.push_back(targPiece);
			_pieceDownloadInterval++;
			_lock.unlock();

			std::stringstream inf;
			inf << "(" << std::fixed << std::setprecision(2) << (((float) _completedPieces.size()) / (float) _totalPieces * 100) << "%) ";
			inf << std::to_string(_completedPieces.size()) + " / " + std::to_string(_totalPieces) + " pieces downloaded";
			LOG_F(INFO, "%s", inf.str().c_str());
		} else {
			targPiece->reset();
			LOG_F(INFO, "Piece %d hash mismatch", targPiece->index);
		}	
		
	}
}

void PieceManager::write(Piece* piece) {
	long position = piece->index * _fileParser.getPieceLen();
	_downloadedFile.seekp(position);
	_downloadedFile << piece->getData();
}

unsigned long PieceManager::bytesDownloaded() {
	_lock.lock();
	unsigned long bytesDownloaded = _completedPieces.size() * _pieceLen;
	_lock.unlock();
	return bytesDownloaded;
}

void PieceManager::trackProgress() {
	usleep(pow(10, 6));
	while(!complete()) {
		displayProgress();
		_pieceDownloadInterval = 0;
		usleep(PROGRESS_INTERVAL * pow(10, 6));
	}
}

void PieceManager::displayProgress() {
	std::stringstream inf;
	_lock.lock();
	unsigned long downloadedPieces = _completedPieces.size();
	unsigned long downloadedLen = _pieceLen * _pieceDownloadInterval;

	double avgDownloadSpeed = (double) downloadedLen / (double) PROGRESS_INTERVAL;
	double avgDownloadSpeedMBS = avgDownloadSpeed / pow(10, 6);

	inf << "Peers: " + std::to_string(_peers.size()) + "/" + std::to_string(_maxConn) + ", ";
	inf << std::fixed << std::setprecision(2) << avgDownloadSpeedMBS << " MB/s, ";

	double timePerPiece = (double) PROGRESS_INTERVAL / (double) _pieceDownloadInterval;
	long remTime = ceil(timePerPiece * (_totalPieces - downloadedPieces));
	inf << "ETA: " << formatTime(remTime) << "";

	double prog = (double) downloadedPieces / (double) _totalPieces;
	int pos = PROGRESS_WIDTH * prog;
	inf << "[";
	for(int i = 0; i < PROGRESS_WIDTH; i++) {
		if(i < pos)
			inf << "=";
		else if(i == pos)
			inf << ">";
		else
			inf << " ";
	}

	inf << "] ";
	inf << std::to_string(downloadedPieces) + "/" + std::to_string(_totalPieces) + " ";
	inf << "[" << std::fixed << std::setprecision(2) << (prog * 100) << "%] ";

	time_t currTime = std::time(nullptr);
	long timeFromStart = floor(std::difftime(currTime, _startTime));

	inf << "in " << formatTime(timeFromStart);
	std::cout << inf.str() <<"\r";
	std::cout.flush();
	_lock.unlock();
	if(complete())
		std::cout << std::endl;
}
