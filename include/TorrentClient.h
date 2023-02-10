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
        const int _threadNum;
        std::string _peerID;
        SharedQueue<Peer*> _queue;
        std::vector<std::thread> _threadPool;
        std::vector<PeerConnection*> _connections;
};


#endif //TORRENTCLIENT_H