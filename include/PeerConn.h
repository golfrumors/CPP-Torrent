#ifndef PEERCONN_H
#define PEERCONN_H

#include "PeerRet.h"
#include "BitMessage.h"
#include "PieceMan.h"
#include "SharedQueue.h"

using byte = unsigned char;

class PeerConnection {
    public:
        const std::string& getPeerID() const;

        explicit PeerConnection(SharedQueue<Peer*>* queue, std::string clientID, std::string infoHash, PieceManager* pieceManager);
        ~PeerConnection();
        void start();
        void stop();
}

#endif // PEERCONN_H