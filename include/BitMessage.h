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
    public:
        explicit BitMessage(uint8_t id, const std::string& payload = "");
        std::string toString();
        uint8_t getMsgId() const;
        std::string getPayload() const;

    private:
        const uint32_t _msgLength;
        const uint8_t _msgID;
        const std::string _payload;
};

#endif // BITMESSAGE_H