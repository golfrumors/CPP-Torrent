#ifndef PIECEMANAGER_H
#define PIECEMANAGER_H

#include <map>
#include <vector>
#include <ctime>
#include <mutex>
#include <fstream>
#include <thread>

#include "Piece.h"
#include "TorrentParser.h"

struct PendingReq {
    Block* block;
    time_t timeStamp;
};

class PieceManager {
    public:
        explicit PieceManager(const TorrentParser& fileParser, const std::string& downloadPth, int maxConn);
        ~PieceManager();
        bool complete();
        void blockReceived(std::string peerID, int pieceIndex, int blockOffset, int blockSize, std::string blockData);
        void addPeer(const std::string& peerID, std::string bitField);
        void removePeer(const std::string& peerID);
        void updatePeer(const std::string& peerID, int index);
        unsigned long bytesDownloaded();
        Block* nextReq(std::string peerID);

    private:
        std::map<std::string, std::string> _peers;
        std::vector<Piece*> _missingPieces;
        std::vector<Piece*> _ongoingPieces;
        std::vector<Piece*> _completedPieces;
        std::vector<PendingReq*> _pendingReqs;
        std::ofstream _downloadedFile;
        const long pieceLen;
        const TorrentParser& _fileParser;
        const int _maxConn;
        int _pieceDownloadInterval = 0;
        time_t _startTime;
        int _totalPieces{};

        //lock to prevent race condition
        std::mutex lock;

        std::vector<Piece*> initiatePieces();
        Block* expiredReq(std::string peerID);
        Block* nextOngoing(std::string peerID);
        Piece* getRarestPiece(std::string peerID);
        void write(Piece* piece);
        void displayProgress();
        void trackProgress();
};

#endif // PIECEMANAGER_H