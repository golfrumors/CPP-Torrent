#ifndef PIECE_H
#define PIECE_H

class Piece {
    public:
        const int index;
        std::vector<Block*> blocks;
        explicit Piece(int index, std::vector<Block*> blocks, std::string hashValue);
        ~Piece();
        void reset();
        std::string getData();
        Block* nextRequest();
        void blockReceived(int offset, std::string data);
        bool isCompleted();
        bool isHashValid();

    private:
        const std::string hashValue;
};

#endif // PIECE_H