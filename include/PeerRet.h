#ifndef PEERRETRIEVER_H
#define PEERRETRIEVER_H

#include <vector>
#include <string>
#include <cpr/cpr.h>

//representation of peers retrieved from tracker
struct Peer{
    std::string ip;
    int port;
};

//sends get requests to tracker to get list of peers
class PeerRetriever{
    public:
        explicit PeerRetriever(std::string peerID, std::string announceURL, std::string infoHash, int port, unsigned long fileSize);
        std::vector<Peer*> getPeers(unsigned long bytesDownloaded = 0);

    private:
        std::string _announceURL;
        std::string _infoHash;
        std::string _peerID;
        int _port;
        const unsigned long _fileSize;
        std::vector<Peer*> decodeResponse(std::string response);
};

#endif // PEERRETRIEVER_H