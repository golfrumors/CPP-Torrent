#include <stdexcept>
#include <iostream>
#include <cassert>
#include <cstring>
#include <chrono>
#include <unistd.h>
#include <netinet/in.h>
#include "../lib/loguru/loguru.hpp"
#include <utility>

#include "include/PeerConn.h"
#include "include/Utils.h"
#include "include/Connect.h"

#define INFO_HASH_START 28
#define PEER_ID_START 48
#define HASH_LEN 20
#define DUMMY_PEER "0.0.0.0"

PeerConn::PeerConn(
    SharedQueue<Peer*>* queue,
    std::string clientID,
    std::string infoHash,
    PieceManager* pieceManager
) : queue(queue), clientID(std::move(clientID)), infoHash(std::move(infoHash)), pieceManager(pieceManager) {}

PeerConn:~PeerConn() {
    closeSocket();
    LOG_F(INFO, "Download was termniated");
}

void PeerConn:start() {
    LOG_F(INFO, "Downloading is started...");
    while(!(terminated || pieceManager->isComplete())){
        peer = queue->pop_front();
        //terminate thread if dummy
        if (peer->ip == DUMMY_PEER) {
            return;
        }

        try {
            //establish connection with peer

            if(establishNewConnection()) {
                while(!pieceManager->isComplete()) {
                    BitMessage msg = receiveMessage();
                    if(msg.getMessageId() > 10)
                        throw std::runtime_error("Invalid message id received from peer " + peer->ip);

                    switch(msg.getMessageId()) {
                        case choke:
                            choked = true;
                            break;

                        case unchoke:
                            choked = false;
                            break;
                        
                        case piece: {
                            requestPending = false;
                            std::string payload = message.getPayload();
                            int pieceIndex = bytesToInteger(payload.substr(0, 4));
                            int blockOffset = bytesToInteger(payload.substr(4, 4));
                            std::string blockData = payload.substr(8);
                            pieceManager->blockReceived(peerID, pieceIndex, blockOffset, blockData);
                            break;
                        }

                        case have: {
                            std::string payload = message.getPayload();
                            int pieceIndex = bytesToInteger(payload);
                            pieceManager->updatePeer(peerID, pieceIndex);
                            break;
                        }

                        default:
                            break;
                        
                    }
                    if (!choked) {
                        if(!requestPending){
                            sendRequest();
                        }
                    }
            }

        }

    }

    catch (std::exception& e) {
        closeSocket();
        LOG_F(ERROR, "Error occured while downloading from peer %s: [%s]", peerID.c_str(), peer->ip.c_str());
        LOG_F(ERROR, "Error: %s", e.what());
    }
}

//terminate connection

void PeerConn::stop() {
    terminated = true;
}

//create TCP conn to peer
void PeerConn::performHandshake() {
    LOG_F(INFO, "Connecting to peer [%s]", peer->ip.c_str());

    try {
        sock - createConnection(peer->ip, peer->port);
    } catch {
        throw std::runtime_error("Failed to connect to peer [" + peer->ip "]");
    }

    LOG_F(INFO, "Established connection with peer [%d]", sock);

    //send handshake
    LOG_F(INFO, "Sending handshake to peer [%s]", peer->ip.c_str());
    std::string handshakeMsg = createHandshakeMessage();

}