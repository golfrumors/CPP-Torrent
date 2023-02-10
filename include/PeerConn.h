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

    private:
        int _socket{};
        bool _choked = true;
        bool _terminated = false;
        bool _requestPending = false;
        const std::string _clinetID;
        const std::string _infoHash;
        SharedQueue<Peer*>* _queue;
        Peer* _peer;
        std::string _peerBitField;
        std::string _peerID;
        PieceManager* _pieceManager;

        std::string createHandshakeMsg();
        void performHandshake();
        void receiveBitField();
        void sendInterested();
        void receiveUnchoke();
        void sendRequest();
        void closeSocket();
        bool connectionEstablished();
        BitMessage receiveMessage(int buffSize = 0) const;


}

#endif // PEERCONN_H