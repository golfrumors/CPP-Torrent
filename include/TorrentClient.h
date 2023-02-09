#ifndef TORRENTCLIENT_H
#define TORRENTCLIENT_H

#include <string>
#include "PeerRet.h"
#include "PeerConn.h"
#include "SharedQueue.h"

class TorrentClient {
    public:
        explicit TorrentClient(int threadNum = 5, bool enableLog = true, std::string logPath = "logs/client.log");
        ~TorrentClient();
        void terminate();
        void downloadFile(const std::string& torrentPath, const std::string& downloadDir);

    private:
        const int threadNum;
        std::string peerID;
        SharedQueue<Peer*> queue;
        std::vector<std::thread> threadPool;
        std::vector<PeerConnection*> connections;
};


#endif //TORRENTCLIENT_H