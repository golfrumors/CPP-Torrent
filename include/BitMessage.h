#ifndef BITMESSAGE_H
#define BITMESSAGE_H

enum MessageId {
    keepAlive = -1,
    choke = 0,
    unchoke = 1,
    interested = 2,
    notInterested = 3,
    have = 4,
    bitfield = 5,
    request = 6,
    piece = 7,
    cancel = 8,
    port = 9
};

class BitMessage {
    private:
        const uint32_t msgLength;
        const uint8_t msgID;
        const std::string payload;

    public:
        explicit BitMessage(uint8_t id, const std::string& payload = "");
        std::string toString();
        uint8_t getMsgId() const;
        std::string getPayload() const;
};

#endif // BITMESSAGE_H