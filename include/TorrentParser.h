#ifndef TORRENTPARSER_H
#define TORRENTPARSER_H

#include <string>
#include <vector>
#include <bencode/BDictionary.h>

class TorrentParser {
    public:
        explicit TorrentParser(const std::string& torrentFile);
        long getFileSize() const;
        long getPieceLen() const;
        std::string getFileName() const;
        std::string getAnnounce() const;
        std::shared_ptr<bencoding::BItem> get(std::string key) const;
        std::string getInfoHash() const;
        std::vector<std::string> splitPieceHashes() const;
};

#endif // TORRENTPARSER_H