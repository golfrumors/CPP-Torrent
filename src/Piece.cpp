#include <utility>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <loguru/loguru.hpp>

#include "../include/Sha.h"
#include "../include/Piece.h"
#include "../include/Utils.h"

Piece::Piece(int index, std::vector<Block*> blocks, std::string hashValue) :
    index(index), _hashValue(std::move(hashValue)) {
        this->blocks = std::move(blocks);
}

Piece::~Piece() {
    for (auto block : blocks) {
        delete block;
    }
}

Block* Piece::nextRequest() {
    for (auto block : blocks) {
        if (block->status == missing) {
            block->status = pending;
            return block;
        }
    }
    return nullptr;
}


void Piece::blockReceived(int offset, std::string data) {
    for (auto block : blocks) {
        if (block->offset == offset) {
            block->data = data;
            block->status = received;
            return;
        }
    }

    throw std::runtime_error("Block does not exist");
}

bool Piece::isCompleted() {
    return std::all_of(blocks.begin(), blocks.end(), [](Block* block) {
        return block->status == received;
    });
}

bool Piece::isHashValid() {
    std::string data = getData();
    std::string hash = hexDecode(sha1(data));
    return hash == _hashValue;
}

std::string Piece::getData() {
    assert(isCompleted());
    std::stringstream ss;

    for (auto block : blocks)
        ss << block->data;
    
    return ss.str();
}