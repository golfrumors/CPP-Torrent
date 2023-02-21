#ifndef BLOCK_H
#define BLOCK_H

#include <string>

enum BlockStat {
    missing = 0,
    pending = 1,
    received = 2
};

//part of piece that is requested & transferred
//by convention it has a length of 2^14 bytes
//and is the smallest unit of data that can be requested
struct Block {
    int piece;
    int offset;
    int length;
    BlockStat status;
    std::string data;
};

#endif // BLOCK_H