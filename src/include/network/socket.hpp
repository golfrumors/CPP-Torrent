/*Header for socket operations
 *Written by: Dmitriy Cherepanov and Soroush Sarram
 *Date: 23/01/2023
 */

#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

//Socket class
class Socket {
	//Public variables and methods
	public:
		Socket();
		~Socket();

		bool connect(const std::string& host, uint16_t port);
		bool send(const std::string& data);
		std::string receive(size_t max_size);
		void close();

	//Private variables and methods
	private:
		int m_sock;
};

//Utility functions
bool resolve_host(const std::string& host, sockaddr_in& addr);
std::string get_ip_str(const sockaddr_in& addr);

#endif
