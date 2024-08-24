#pragma once
#include <WinSock2.h>
#include <memory>
#include "Tool.h"
#include "Packet.h"

enum class MTYPE {
	MTypeTCP = 1,
	MTypeUDP
};

class MSockaddrIn
{
public:
	MSockaddrIn() {
		memset(&m_addr, 0, sizeof(m_addr));
		m_port = -1;
	}

	MSockaddrIn(sockaddr_in addr) {
		memcpy(&m_addr, &addr, sizeof(addr));
		m_ip = inet_ntoa(addr.sin_addr);			//inet_ntoa 将 32 位的 IP 地址（以网络字节序表示）转换为点分十进制字符串格式。
		m_port = ntohs(addr.sin_port);
	}

	MSockaddrIn(UINT nIP, short nPort) {
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = htonl(nIP);

		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = nPort;
	}

	MSockaddrIn(const std::string& strIP, short nPort) {
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = inet_addr(strIP.c_str());

		m_ip = strIP;
		m_port = nPort;
	}

	MSockaddrIn(const MSockaddrIn& addr) {
		memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
		m_ip = addr.m_ip;
		m_port = addr.m_port;
	}

	MSockaddrIn& operator=(const MSockaddrIn& addr) {
		if (this != &addr) {
			memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
			m_ip = addr.m_ip;
			m_port = addr.m_port;
		}
		return *this;
	}

	operator sockaddr* () const {
		return (sockaddr*)&m_addr;
	}

	operator void* () const {
		return (void*)&m_addr;
	}

	operator sockaddr_in() const {
		return m_addr;
	}

	void update() {
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}

	std::string GetIP() const { return m_ip; };
	short GetPort() const { return m_port; }

	int size() const { return sizeof(sockaddr); }
	 
private:
	sockaddr_in m_addr;
	std::string m_ip;
	short m_port;
};

class MBuffer : public std::string
{
public:
	MBuffer(size_t size = 0) :std::string() {
		if (size > 0) { 
			resize(size); 
			memset(*this, 0, this->size());
		}
	}

	MBuffer(const char* str) {
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}

	MBuffer(void* buffer, size_t size) :std::string() {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}

	~MBuffer() {
		std::string::~basic_string();
	}

	operator char* () const { return (char*)c_str(); }
	operator const char* () const { return c_str(); }
	operator BYTE* () const { return (BYTE*)c_str(); }
	operator void* () const { return (void*)c_str(); }

	MBuffer operator=(const char* str) {
		std::string::operator=(str);
		return *this;
	}
	
	void Updata(void* buffer, size_t size) {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}

private:

};

class MSocket
{
public: 
	MSocket(MTYPE nType = MTYPE::MTypeTCP, int nProtocol = 0){
		m_socket = socket(PF_INET, (int)nType, nProtocol);
		m_type = nType;
		m_Protocol = nProtocol;
	}

	~MSocket() {
		close();
	}

	MSocket(const MSocket& sock) {
		m_socket = socket(PF_INET, (int)sock.m_type, sock.m_Protocol);
		m_type = sock.m_type;
		m_Protocol = sock.m_Protocol;
		m_addr = sock.m_addr;
	}

	MSocket& operator= (const MSocket& sock) {
		if (this != &sock) {
			m_socket = socket(PF_INET, (int)sock.m_type, sock.m_Protocol);
			m_type = sock.m_type;
			m_Protocol = sock.m_Protocol;
			m_addr = sock.m_addr;
		}
		return *this;
	}

	bool operator==(SOCKET sock) const {
		return m_socket == sock;
	}

	operator SOCKET() { return m_socket; }
	operator SOCKET() const { return m_socket; }

	int bind(const std::string& ip, short port) {
		m_addr = MSockaddrIn(ip, port);
		return ::bind(m_socket, m_addr, m_addr.size());
	}

	int listen(int backlog = 5) {
		if (m_type != MTYPE::MTypeTCP)return -1;
		return ::listen(m_socket, backlog);
	}

	int accept(){}

	int connect(const std::string& ip, short port) {}

	int send(const MBuffer& buffer) {
		return ::send(m_socket, buffer, buffer.size(), 0);
	}

	int recv(MBuffer& buffer) {
		return ::recv(m_socket, buffer, buffer.size(), 0);
	}

	int sendto(const MBuffer& buffer, MSockaddrIn& to){
		//CTool::Dump((BYTE*)&buffer, sizeof(buffer));
		return ::sendto(m_socket, buffer, buffer.size(), 0, to, to.size());
	}

	int sendto(CPacket& pack, MSockaddrIn& to) {
		return ::sendto(m_socket, (char*)pack.Data(), pack.Size(), 0, to, to.size());
	}

	int recvfrom(MBuffer& buffer, MSockaddrIn& from){
		int len = from.size();
		int ret = ::recvfrom(m_socket, buffer, buffer.size(), 0, from, &len);
		if (ret > 0) {
			from.update();
		}
		return ret;
	}

	void close() {
		if (m_socket != INVALID_SOCKET) {
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
	}

private:
	SOCKET m_socket;
	MTYPE m_type;
	int m_Protocol;
	MSockaddrIn m_addr;
};


typedef std::shared_ptr<MSocket> MSOCKET;
