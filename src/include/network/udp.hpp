//header for the UDP protocol
#ifndef UDP_HPP
#define UDP_HPP

#include "socket.hpp"

class UDPSocket : public Socket {
	public:
		UDPSocket();
		~UDPSocket();

		bool bind(uint16_t port);
		bool send_to(const std::string& host, uint16_t port, const std::string& data);
		std::string receive_from(size_t max_length, std::string& host, uint16_t& port);
};

#endif

