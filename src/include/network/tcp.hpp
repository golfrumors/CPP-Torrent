//header for the TCP protocol

#ifndef TCP_HPP
#define TCP_HPP

#include "socket.hpp"

class TCPConnection : public Socket {
	public:
		TCPConnection();
		~TCPConnection();

		bool connect(const std::string& host, uint16_t port);
		bool send(const std::string& data);
		std::string receive(size_t max_length);
};

class TCPServer {
	public:
		TCPServer();
		~TCPServer();

		bool listen(uint16_t port, int backlog = SOMAXCONN);
		TCPConnection accept();
};

#endif
