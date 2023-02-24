#include <random>
#include <iostream>
#include <thread>
#include <bencode/bencoding.h>
#include <loguru/loguru.hpp>

#include "../include/TorrentClient.h"
#include "../include/PieceMan.h"
#include "../include/TorrentParser.h"
#include "../include/PeerRet.h"
#include "../include/PeerConn.h"

#define PORT 8080
#define PEER_QUER_INT 60

TorrentClient::TorrentClient(const int threadNum, bool enableLog, std::string logFilePath) : _threadNum(threadNum) {
	_peerID = "-UT2023-";

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, 9);
	for (int i = 0; i < 12; i++) {
		_peerID += std::to_string(dis(gen));
	}
	
	if(enableLog) {
		loguru::g_stderr_verbosity = loguru::Verbosity_OFF;
		loguru::g_flush_interval_ms = 100;
		loguru::add_file(logFilePath.c_str(), loguru::Append, loguru::Verbosity_MAX);
	} else {
		loguru::g_stderr_verbosity = loguru::Verbosity_OFF;
	}
}

TorrentClient::~TorrentClient() = default;

void TorrentClient::downloadFile(const std::string& torrentFilePath, const std::string& downloadDir) {
	std::cout << "Parsing torrent file " + torrentFilePath + "..." << std::endl;
	TorrentParser torrentFileParser(torrentFilePath);
	std::string announceURL = torrentFileParser.getAnnounce();

	long fileSize = torrentFileParser.getFileSize();
	const std::string infHash = torrentFileParser.getInfoHash();

	std::string fileName = torrentFileParser.getFileName();
	std::string downloadFilePath = downloadDir + fileName;
	PieceManager pieceMan(torrentFileParser, downloadFilePath, _threadNum);

	for(int i = 0; i < _threadNum; i++) {
		PeerConnection connection(&_queue, _peerID, infHash, &pieceMan);
		//_connections.push_back(std::move(connection));
		_connections.push_back(&connection);
		std::thread thread(&PeerConnection::start, connection);
		_threadPool.push_back(std::move(thread));
	}

	auto lastPeerQuery = (time_t) (-1);

	std::cout << "Download started..." << std::endl;

	while(true) {
		if(pieceMan.complete())
			break;

		time_t currTime = std::time(nullptr);
		auto diff = std::difftime(currTime, lastPeerQuery);

		if(lastPeerQuery == -1 || diff >= PEER_QUER_INT || _queue.empty()) {
			PeerRetriever peerRetriever(_peerID, announceURL, infHash, PORT, fileSize);
			std::vector<Peer*> peers = peerRetriever.getPeers(pieceMan.bytesDownloaded());
			lastPeerQuery = currTime;
			if(!peers.empty()) {
				_queue.clear();
				for(auto peer : peers) {
					_queue.push_back(peer);
				}
			}
		}

		terminate();
		
		if(pieceMan.complete()) {
			std::cout << "Download completed" << std::endl;
			std::cout << "File downloaded to " + downloadFilePath << std::endl;
		}
	}
}

void TorrentClient::terminate() {
	for(int i = 0; i < _threadNum; i++) {
		Peer* dummyPeer = new Peer {"0.0.0.0", 0};
		_queue.push_back(dummyPeer);
	}

	for(auto connection : _connections)
		connection->stop();

	for(std::thread& thread : _threadPool) {
		if(thread.joinable())
			thread.join();
	}

	_threadPool.clear();
}
