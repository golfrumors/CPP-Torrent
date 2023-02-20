#include <stdexcept>
#include <string>
#include <iostream>
#include <cassert>
#include <cstring>
#include <chrono>
#include <unistd.h> //used on Linux, need to find workaround for Windows
#include <netinet/in.h> //used on Linux, need to find workaround for Windows
#include <utility>
#include <malloc.h>

#include "../include/PeerConn.h"
#include "../include/Utils.h"
#include "../include/Connect.h"

#include <loguru/loguru.hpp>


#define INFO_HASH_START 28
#define PEER_ID_START 48
#define HASH_LEN 20
#define DUMMY_PEER "0.0.0.0"

PeerConnection::PeerConnection(
    SharedQueue<Peer*>* queue,
    std::string clientID,
    std::string infoHash,
    PieceManager* pieceManager
) : _queue(queue), _clientID(std::move(clientID)), _infoHash(std::move(infoHash)), _pieceManager(pieceManager) {}

PeerConnection::~PeerConnection() {
    closeSocket();
    LOG_F(INFO, "Download was termniated");
}

void PeerConnection::start() {
    LOG_F(INFO, "Downloading is started...");
    while(!(_terminated || _pieceManager->complete())){
        _peer = _queue->pop_front();
        //terminate thread if dummy
        if (_peer->ip == DUMMY_PEER) {
            return;
        }

        try {
            //establish connection with peer

            if(connectionEstablished()) {
                while(!_pieceManager->complete()) {
                    BitMessage msg = receiveMessage();
                    if(msg.getMsgId() > 10)
                        throw std::runtime_error("Invalid message id received from peer " + _peer->ip);

                    switch(msg.getMsgId()) {
                        case choke:
                            _choked = true;
                            break;

                        case unchoke:
                            _choked = false;
                            break;
                        
                        case piece: {
                            _requestPending = false;
                            std::string payload = msg.getPayload();
                            int pieceIndex = bytesToInteger(payload.substr(0, 4));
                            int blockOffset = bytesToInteger(payload.substr(4, 4));
                            std::string blockData = payload.substr(8);
                            _pieceManager->blockReceived(_peerID, pieceIndex, blockOffset, blockData);
                            break;
                        }

                        case have: {
                            std::string payload = msg.getPayload();
                            int pieceIndex = bytesToInteger(payload);
                            _pieceManager->updatePeer(_peerID, pieceIndex);
                            break;
                        }

                        default:
                            break;
                        
                    }
                    if (!_choked) {
                        if(!_requestPending){
                            sendRequest();
                        }
                    }
            }

        }

    }

    catch (std::exception& e) {
        closeSocket();
        LOG_F(ERROR, "Error occured while downloading from peer %s: [%s]", _peerID.c_str(), _peer->ip.c_str());
        LOG_F(ERROR, "Error: %s", e.what());
    }
}
}

//terminate connection

void PeerConnection::stop() {
    _terminated = true;
}

//create TCP conn to peer
void PeerConnection::performHandshake() {
    LOG_F(INFO, "Connecting to peer [%s]", _peer->ip.c_str());

    try {
        _socket = createConnection(_peer->ip, _peer->port);
    } catch (std::runtime_error &e) {
        throw std::runtime_error("Failed to connect to peer [" + _peer->ip + "]");
    }

    LOG_F(INFO, "Established connection with peer [%d]", _socket);

    //send handshake
    LOG_F(INFO, "Sending handshake to peer [%s]", _peer->ip.c_str());
    std::string handshakeMsg = createHandshakeMsg();
    sendData(_socket, handshakeMsg);
    LOG_F(INFO, "Handshake sent to peer [%s], success!", _peer->ip.c_str());

    //receive handshake
    LOG_F(INFO, "Receiving handshake from peer [%s]", _peer->ip.c_str());
    std::string receivedHandshake = receiveData(_socket, handshakeMsg.length());
    if(receivedHandshake.empty()) {
        throw std::runtime_error("Failed to receive handshake from peer [" + _peer->ip + "]");
    }
    _peerID = receivedHandshake.substr(PEER_ID_START, HASH_LEN);
    LOG_F(INFO, "Handshake received from peer [%s], success!", _peer->ip.c_str());

    //compare hash
    std::string receivedHash = receivedHandshake.substr(INFO_HASH_START, HASH_LEN);
    if((receivedHash.compare(_infoHash)) != 0) {
        throw std::runtime_error("Hashes do not match");
    }
    LOG_F(INFO, "Hashes match, success!");

}

//read bitfield from peer
void PeerConnection::receiveBitField() {
    LOG_F(INFO, "Receive message from peer [%s]", _peer->ip.c_str());
    BitMessage msg = receiveMessage();

    if(msg.getMsgId() != bitfield) {
        throw std::runtime_error("Expected bitfield message from peer [" + _peer->ip + "]");
    }
    _peerBitField = msg.getPayload();

    //update piece manager
    _pieceManager->addPeer(_peerID, _peerBitField);

    LOG_F(INFO, "Received bitfield from peer [%s], success!", _peer->ip.c_str());
}

//send request to peer for block
void PeerConnection::sendRequest() {
    Block* block = _pieceManager->nextReq(_peerID);

    if(block == nullptr) {
        return;
    }

    //make sure the memory is aligned properly beforehand
    size_t alignment = 32;
    const size_t payLoadSize = 12;
    char* tmp[payLoadSize] = (char*)memalign(alignment, payLoadSize);

    //convert little endian to big endian
    uint32_t pieceIndex = htonl(block->piece);
    uint32_t blockOffset = htonl(block->offset);
    uint32_t blockSize = htonl(block->length);

    //call for fast memcpy, using g++ compile flags, otherwise, use regular memcpy
    if(__builtin_cpu_supports("avx2")) {
        fastMemcpy(tmp, &pieceIndex, sizeof(int));
        fastMemcpy(tmp + 4, &blockOffset, sizeof(int));
        fastMemcpy(tmp + 8, &blockSize, sizeof(int));
    } else {
        memcpy(tmp, &pieceIndex, sizeof(int));
        memcpy(tmp + 4, &blockOffset, sizeof(int));
        memcpy(tmp + 8, &blockSize, sizeof(int));
    }

    std::string payload;
    for(int i = 0; i < payLoadSize; i++) {
        payload += (char) tmp[i];
    }

    std::stringstream ss;
    ss << "Sending request to peer [" << _peer->ip << "]";
    ss << " for piece [" << std::to_string(block->piece) << "]";
    ss << " at offset [" << std::to_string(block->offset) << "]";
    ss << " with length [" << std::to_string(block->length) << "]";

    LOG_F(INFO, "%s", ss.str().c_str());

    std::string requestMsg = BitMessage(request, payload).toString();

    sendData(_socket, requestMsg);

    _requestPending = true;

    LOG_F(INFO, "Request sent to peer [%s], success!", _peer->ip.c_str());

    //dealloc
    free(tmp);
}

void PeerConnection::sendInterested() {
    LOG_F(INFO, "Sending interested message to peer [%s]", _peer->ip.c_str());
    std::string interestedMsg = BitMessage(interested).toString();
    sendData(_socket, interestedMsg);
    LOG_F(INFO, "Interested message sent to peer [%s], success!", _peer->ip.c_str());
}

void PeerConnection::receiveUnchoke() {
    LOG_F(INFO, "Receiving unchoke message from peer [%s]", _peer->ip.c_str());
    BitMessage msg = receiveMessage();

    if(msg.getMsgId() != unchoke) {
        throw std::runtime_error("Expected unchoke message from peer [" + _peer->ip + "]");
    }
    _choked = false;
    LOG_F(INFO, "Received unchoke message from peer [%s], success!", _peer->ip.c_str());
}

//attempt to establis a new connection
bool PeerConnection::connectionEstablished() {
    try {
        performHandshake();
        receiveBitField();
        sendInterested();
        return true;
    } catch (std::exception& e) {
        LOG_F(ERROR, "Error occured while establishing connection with peer [%s]", _peer->ip.c_str());
        LOG_F(ERROR, "Error: %s", e.what());
        return false;
    }
}

std::string PeerConnection::createHandshakeMsg() {
    const std::string pstr = "BitTorrent protocol";

    std::stringstream ss;

    ss << (char) pstr.length();
    ss << pstr;

    std::string res;

    for(int i = 0; i < 8; i++) {
        res.push_back('\0');
    }

    ss << res;
    ss << hexDecode(_infoHash);
    ss << _clientID;

    assert (ss.str().length() == pstr.length() + 49);

    return ss.str();
}

BitMessage PeerConnection::receiveMessage(int bufferSize) const {
    std::string reply = receiveData(_socket, bufferSize);

    if(reply.empty()) {
        return BitMessage(keepAlive);
    }

    auto msgId = (uint8_t) reply[0];
    std::string payload = reply.substr(1);
    LOG_F(INFO, "Received message from peer [%s]", _peer->ip.c_str());
    return BitMessage(msgId, payload);
}

const std::string& PeerConnection::getPeerID() const {
    return _peerID;
}

void PeerConnection::closeSocket() {
    if(_socket) {
        LOG_F(INFO, "Closing socket for peer [%s]", _peer->ip.c_str());
        close(_socket);
        _socket = {};
        _requestPending = false;

        //remove from piece manager
        if (!_peerBitField.empty()) {
            _peerBitField.clear();
            _pieceManager->removePeer(_peerID);
        }

    }
}
