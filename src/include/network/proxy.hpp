//header for proxy support

#ifndef PROXY_HPP
#define PROXY_HPP

#include <string>

class Proxy {
	public:
		Proxy();
		~Proxy();
		bool set_type(const std::string& type);
		bool set_host(const std::string& host);
		bool set_port(uint16_t port);
		bool set_username(const std::string& username);
		bool set_password(const std::string& password);
		std::string get_type() const;
		std::string get_host() const;
		uint16_t get_port() const;
		std::string get_username() const;
		std::string get_password() const;

	private:
		std::string m_type;
		std::string m_host;
		uint16_t m_port;
		std::string m_username;
		std::string m_password;
};

#endif
