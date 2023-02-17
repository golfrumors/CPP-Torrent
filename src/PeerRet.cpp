#include "Utils.h"
#include "PeerRet.h"

#include "../lib/loguru/loguru.hpp"
#include "../lib/bencode/bencoding.h"

#include <string>
#include <iostream>
#include <cpr/cpr.h>
#include <random>
#include <stdexcept>
#include <bitset>
#include <utility>

#define TIMEOUT 15000

PeerRetriever::PeerRetriever(std::string peerID, std::string announceURL, std::string infoHash, int port, const unsigned long fileSize):  _fileSize(fileSize) {
    this->_peerID = peerID;
    this->_announceURL = announceURL;
    this->_infoHash = infoHash;
    this->_port = port;
}

std::vector<Peer*> PeerRetriever::getPeers(unsigned long bytesDown) {
    std::stringstream ss;
    ss << "Getting peers from" << _announceURL << "with params:\n";
    ss << "info hash: " << _infoHash << "\n" << "peer id : " << _peerID << "\n";
    ss << "port: " << _port << "\n" << "uploaded: " << 0 << "\n";
    ss << "downloaded: " << bytesDown << "\n" << "left: " << std::to_string(_fileSize - bytesDown) << "\n";
    ss << "compact: " << std::to_string(1) << "\n" << "event: " << "started" << "\n";

    LOG_F(INFO, "%s", info.str().c_str());

    cpr::Response res = cpr::Get(cpr::Url{_announceURL}, cpr::Parameters {
        {"info_hash", std::string(hexDecode(_infoHash))},
        {"peer_id", std::string(_peerID)},
        {"port", std::to_string(_port)},
        {"uploaded", std::to_string(0)},
        {"downloaded", std::to_string(bytesDown)},
        {"left", std::to_string(_fileSize - bytesDown)},
        {"compact", std::to_string(1)},
    }, cpr::Timeout{TIMEOUT});

    if (res.status_code == 200) {
        LOG_F(INFO, "Got peers from tracker: success");
        std::vector<Peer*> peers = decodeResponse(res.text);
        return peers;
    } else {
        LOG_F(ERROR, "Error getting peers from tracker: %d : %s", res.status_code, res.text.c_str());
    }

    return std::vector<Peer*>();
}

std::vector<Peer*> PeerRetriever::decodeResponse(std::string resp) {
    LOG_F(INFO, "Decoding response from tracker...");
    std::shared_ptr<bencoding::BItem> decodedResp = bencoding::decode(resp);

    std::shared_ptr<bencoding::BDictionary> respDict = std::dynamic_pointer_cast<bencoding::BDictionary>(decodedResp);
    std::shared_ptr<bencoding::BItem> peersVal = respDict->getValue("peers");

    if(!peersVal)
        throw std::runtime_error("No peers in response returned from tracker");

    std::vector<Peer*> peers;

	if(typeid(*peersValue) == typeid(bencoding::BString)) {
		const int peerInfSize = 6;
		std::string peersString = std::dynamic_pointer_cast<bencoding::BString>(peersVal)->value();

	}    

}
