#include <iostream>
#include <sstream>
#include <bitset>

#include "../include/BitMessage.h"

BitMessage::BitMessage(const uint8_t id, const std::string& payload) : 
    _msgID(id), _payload(payload), _msgLength(payload.length() + 1) {}


std::string BitMessage::toString() {
    std::stringstream buffer;

    char* msgLengthAddr = (char*)&_msgLength;
    std::string msgLengthStr;

    //bytes pushed in reverse order, assuming little endian
    for (int i = 0; i < 4; i++)
        msgLengthStr.push_back((char)msgLengthAddr[3-i]);
    
    buffer << msgLengthStr;
    buffer << (char) _msgID;
    buffer << _payload;

    return buffer.str();
}

//getter for atrtibute msgID
uint8_t BitMessage::getMsgId() const {
    return _msgID;
}

//getter for attribute payload
std::string BitMessage::getPayload() const {
    return _payload;
}