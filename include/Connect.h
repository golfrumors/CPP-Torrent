#ifndef CONNECT_H
#define CONNECT_H

#include <string>

//funcs to handle network conns

int createConnection(const std::string& ip, int port);
void sendData(int socket, const std::string& data);
std::string receiveData(int socket, uint32_t buffSize = 0);

#endif // CONNECT_H