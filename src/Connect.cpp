#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/poll.h>
#include <fcntl.h>
//all the ones above are unix specific, need to find windows equivalents
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <chrono>
#include <limits>

#include <loguru/loguru.hpp>

#include "../include/Connect.h"
#include "../include/Utils.h"

#define CONN_TIMEOUT 5
#define READING_TIMEOUT 3000

bool setBlockSocket(int socket, bool blocking) {
    if (socket < 0)
        return false;

    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1)
        return false;

    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(socket, F_SETFL, flags) == 0);
}

int createConnection(const std::string& ip, const int port) {
    int sock = 0;

    struct sockaddr_in add;

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw std::runtime_error("Socket creation error");

    add.sin_family = AF_INET;
    add.sin_port = htons(port);

    char* tmpIP = new char[ip.length() + 1];
    //@HERE this might??? explode idk
    fastStrCopy(tmpIP, ip.length() + 1, ip);

    if(inet_pton(AF_INET, tmpIP, &add.sin_addr) <= 0)
        throw std::runtime_error("Invalid address or address not supported");

    if(!setBlockSocket(sock, false))
        throw std::runtime_error("Failed to set socket to non-blocking");

    connect(sock, (struct sockaddr *)&add, sizeof(add));

    fd_set fdset;
    struct timeval tv;
    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);
    tv.tv_sec = CONN_TIMEOUT;
    tv.tv_usec = 0;

    if(select(sock + 1, NULL, &fdset, NULL, &tv) == 1) {
        int socket_error;
        socklen_t length = sizeof socket_error;

        getsockopt(sock, SOL_SOCKET, SO_ERROR, &socket_error, &length);

        if(socket_error != 0) {
            throw std::runtime_error("Connection failed");
        }
    
        if(!setBlockSocket(sock, true))
            throw std::runtime_error("Failed to set socket to blocking");
        
        return sock;

    }

    close(sock);
    throw std::runtime_error("Connection timed out");
}

void sendData(const int sock, const std::string& data) {
    int n = data.length();
    char buff[n];

    for(int i = 0; i < n; i++) {
        buff[i] = data[i];
    }

    int res = send(sock, buff, n, 0);

    if(res < 0)
        throw std::runtime_error("Failed to send data");
}

std::string receiveData(const int sock, uint32_t bufferSize) {
    std::string ans;

    if (!bufferSize) {
        struct pollfd fds;
        int ret;
        fds.fd = sock;
        fds.events = POLLIN;
        ret = poll(&fds, 1, READING_TIMEOUT);

        long bytesRead;
        const int lenSize = 4;
        char buffer[lenSize];

        switch(ret) {
            case -1:
                throw std::runtime_error("Error while polling");
            case 0:
                throw std::runtime_error("Reading timed out");
            default:
                bytesRead = recv(sock, buffer, sizeof(buffer), 0);
        }

        if (bytesRead != lenSize)
            return ans;

        std::string msgLenString;
        for(char i : buffer)
            msgLenString += i;

        uint32_t msgLen = bytesToInt(msgLenString);
        bufferSize = msgLen;
    }

    if(bufferSize > std::numeric_limits<int>::max())
        throw std::runtime_error("Buffer size too big");

    char buffer[bufferSize];

    long bytesRead = 0;
    long bytesToRead = bufferSize;
    
    auto startTime = std::chrono::steady_clock::now();
    do {
        auto difference = std::chrono::steady_clock::now() - startTime;

        if(std::chrono::duration<double, std::milli> (difference).count() > READING_TIMEOUT)
            throw std::runtime_error("Reading timed out");

        bytesRead = recv(sock, buffer, bufferSize, 0);

        if(bytesRead <= 0)
            throw std::runtime_error("Failed to read data");

        bytesToRead -= bytesRead;

        for(int i = 0; i < bytesRead; i++)
            ans.push_back(buffer[i]);

    } while(bytesToRead > 0);

    return ans;
}