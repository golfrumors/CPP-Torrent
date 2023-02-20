#include <cmath>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <loguru/loguru.hpp>
#include <bencode/bencoding.h>
#include <iomanip>
#include <unistd.h>

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
): fileParser(fileParse), maxConn(maxConn), pieceLength(fileParse.getPieceLen())
{
	missingPieces = initiatePieceS();

	downloadedFile.open(dwnPath, std::ios::binary | std::ios::out);
	downloadedFile.seekp(fileParse.getFileSize() - 1);
	downloadedFile.write("", 1);

	startingTime = std::time(nullptr);
	std::thread progressThread([this] {this->trackProgress();});

	progressThread.detach();
}

PieceManager::~PieceManager() {
	for (Piece* piece : missingPieces)
		delete piece;

	for (Piece* piece : ongoingPiece)
		delete piece;

	for (PendingRequest* req : pendingRequests)
		delete req;

	downloadedFile.close();
}

std::vector<Piece*> PieceManager::initiatePieces() {
	std::vector<std::string> pieceHashes = fileParser.splitPieceHashes();
	numPieces = pieceHashes.size();
	std::vector<Piece*> torPieces;
	missingPieces.reserve(numPieces);

	long totalLen = fileParser.getFileSize();

	int blockCount = ceil(pieceLength / BLOCK_SZ);
	long remLen = pieceLength;

	for(int i = 0; i < totalPieces; i++) {
		if(i == totalPieces - 1) {
			remLen = totalLen % pieceLength;
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

			if (i == totalPieces - 1 && off == blockCount - 1)
				blockSz = remLen % BLOCK_SZ;

			block->length = blockSz;
			blocks.push_back(block);
		}

		auto piece = new Piece(i, blocks, pieceHashes[i]);
		torPieces.push_back(piece);

	}

	return torPieces;
}

bool PieceManager::isComplete() {
	lock.lock();
	bool isComplete = havePieces.size() == totalPieces;
	lock.unlock();
	return isComplete;
}

void PieceManager::addPeer(const std::string& peerID, std::string bitField) {
	lock.lock();
	peers[peerID] = bitField;
	lock.unlock();
	std::stringstream inf;
	inf << "Num of cons: " <<
		std::to_string(peers.size()) << "/" << std::to_string(maxConn);
	LOG_F(INFO, "%s",inf.str().c_str());
}

void PieceManager::updatePeer(const std::string& peerID, int ind) {
	lock.lock();
	if(peers.find(peerID) != peers.end()) {
		setPiece(peers[peerID],ind);
		lock.unlock();
	} else { 
		lock.unlock();
		throw std::runtime_error("Connection failed with peer " + peerID);
	}
}

void PieceManager::removePeer(const std::string& peerID) {
	if(isComplete())
		return;
	lock.lock();
	auto it = peers.find(peerID);
	if (it != peers.end()) {
		peers.erase(it);
		lock.unlock();
		std:stringstream inf;
		inf << "Num of cons: " <<
			std::to_string(peers.size()) << "/" << std::to_string(maxConn);
		LOG_F(INFO, "%s", inf.str().c_str());
	} else {
		lock.unlock();
		throw std::runtime_error("Trying to remove a peer: " + peerID + " that doesn't exist");
	}
}

Block* PieceManager::nextRequest(std::string peerID){
	lock.lock();
	if(missingPieces.empty()) {
		lock.unlock();
		return nullptr;
	}

	if(peers.find(peerID) == peers.end()) {
		lock.unlock();
		return nullptr;
	}

	Block* block = expiredRequest(peerID);
	if(!block) {
		block = nextOngoing(peerID);
		if(!block)
			block = getRarestPiece(peerID)->nextRequest();
	}

	lock.unlock();

	return block;
}

Block* PieceManager::expiredRequest(std::string peerID) {
	time_t curr = std::time(nullptr);

	for(auto it = pendingRequests.begin(); it != pendingRequests.end(); it++) {
		if((*it)->peerID == peerID && curr - (*it)->time > PENDING_TIME) {
			Block* block = (*it)->block;
			pendingRequests.erase(it);
			return block;
		}
	}

	return nullptr;
}

Block* PieceManager::nextOngoing(std::string peerID) {
	for(auto it = ongoingPiece.begin(); it != ongoingPiece.end(); it++) {
		if((*it)->hasPeer(peerID)) {
			Block* block = (*it)->nextRequest();
			if(block)
				return block;
		}
	}

	return nullptr;
}

Piece* PieceManager::getRarestPiece(std::string peerID) {
	std::vector<Piece*> rarestPieces;
	int rarestCount = INT_MAX;

	for(Piece* piece : missingPieces) {
		if(piece->hasPeer(peerID)) {
			int count = 0;
			for(auto it = peers.begin(); it != peers.end(); it++) {
				if(it->first != peerID && piece->hasPeer(it->first))
					count++;
			}

			if(count < rarestCount) {
				rarestPieces.clear();
				rarestPieces.push_back(piece);
				rarestCount = count;
			} else if(count == rarestCount) {
				rarestPieces.push_back(piece);
			}
		}
	}

	return rarestPieces[rand() % rarestPieces.size()];
}

void PieceManager::blockReceived(std::string peerID, int pieceInd, int blockOff, std::string data) {
	LOG_F(INFO, "Block %d received from %s", blockOff, peerID.c_str());

	PendingRquest* pendingToRem = nullptr;
	lock.lock();
	for(PendingRequest* pending : pendingRequests) {
		if(pending->block->piece == pieceInd && pending->block->offset == blockOff) {
			pendingToRem = pending;
			break;
		}
	}

	pendingRequests.erase(
			std::remove(pendingRequests.begin(), pendingRequests.end(), pendingToRem),
			pendingRequests.end());
	
	Piece* targPiece = nullptr;
	for(Piece* piece : ongoingPiece) {
		if(piece->index == pieceInd) {
			targetPiece = piece;
			break;
		}
	}

	lock.unlock();
	if(!targetPiece)
		throw std::runtime_error("Received block for a piece that is not ongoing");

	targetPiece->blockReceived(blockOffset, std::move(data));

	if(targetPiece->isComplete()) {
		if(targetPiece->isHashMatch()) {
			write(targetPiece);

			lock.lock();
			ongoingPieces.erase(
				std::remove(ongoingPieces.begin(), ongoingPieces.end(), targetPiece),
				ongoingPieces.end());

			havePieces.push_back(targetPiece);
			piecesDownloadedInInterval++;
			lock.unlock();

			std::stringstream inf;
			inf << "(" << std::fixed << std::setprecision(2) << (((float) havePieces.size()) / (float) totalPieces * 100) << "%) ";
			inf << std::to_string(havePieces.size()) + " / " + std::to_string(totalPieces) + " pieces downloaded";
			LOG_F(INFO, "%s", inf.str().c_str());
		} else {
			targetPiece->reset();
			LOG_F(INFO, "Piece %d hash mismatch", targetPiece->index);
		}	
		
	}
}

void PieceManager::write(Piece* piece) {
	long position = piece->index * fileParser.getPieceLength();
	downloadedFile.seekp(position);
	downloadedFile << piece->getData();
}

unsigned long PieceManager::bytesDownloaded() {
	lock.lock();
	unsigned long bytesDownloaded = havePieces.size() * pieceLength();
	lock.unlock();
	return bytesDownloaded;
}

void PieceManager::trackProgress() {
	usleep(pow(10, 6));
	while(!isComplete()) {
		displayProgressBar();
		piecesDownloadedInInterval = 0;
		usleep(PROGRESS_INTERVAL * pow(10, 6));
	}
}

void PieceManager::displayProgressBar() {
	std::stringstream inf;
	lock.lock();
	unsigned long downloadedPieces = havePieces.size();
	unsigned long downloadedLen = pieceLength * piecesDownloadedInInterval;

	double avgDownloadSpeed = (double) downloadedLen / (double) PROGRESS_INTERVAL;
	double avgDownloadSpeedMBS = avgDownloadSpeed / pow(10, 6);

	inf << "Peers: " + std::to_string(peers.size()) + "/" + std::to_string(MAX_PEERS) + ", ";
	inf << std::fixed << std::setprecision(2) << avgDownloadSpeedMBS << " MB/s, ";

	double timePerPiece = (double) PROGRESS_INTERVAL / (double) piecesDownloadedInInterval;
	long remTime = ceil(timePerPiece * (totalPieces - downloadedPieces));
	inf << "ETA: " << formatTime(remTime) << "";

	double prog = (double) downloadedPieces / (double) totalPieces;
	int pos = PROGRESS_BAR_WIDTH * prog;
	inf << "[";
	for(int i = 0; i < PROGRESS_BAR_WIDTH; i++) {
		if(i < pos)
			inf << "=";
		else if(i == pos)
			inf << ">";
		else
			inf << " ";
	}

	inf << "] ";
	inf << std::to_string(downloadedPieces) + "/" + std::to_string(totalPieces) + " ";
	inf << "[" << std::fixed << std::setprecision(2) << (prog * 100) << "%] ";

	time_t currTime = std::time(nullptr);
	long timeFromStart = floor(std::difftime(currTime, startTime));

	inf << "in " << formatTime(timeFromStart);
	std::cout << inf.str() <<"\r";
	std::cout.flush();
	lock.unlock();
	if(isComplete())
		std::cout << std::endl;
}
