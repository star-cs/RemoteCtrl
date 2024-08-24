#pragma once
#include "MyThread.h"
#include "MSocket.h"
#include <map>

class CMNetwork
{

};

typedef int (*AcceptFunc)(void* arg, MSOCKET& client);
typedef int(*RecvFunc)(void* arg, const MBuffer& buffer);
typedef int(*SendFunc)(void* arg, MSOCKET& client, int ret);
typedef int(*RecvFromFunc)(void* arg, MBuffer& buffer, MSockaddrIn& addr);
typedef int(*SendToFunc)(void* arg, const MSockaddrIn& addr, int ret);


class MServerParamter
{
public:
	MServerParamter(){}

	MServerParamter(
		const std::string& ip,
		short port = 9527,
		MTYPE type = MTYPE::MTypeTCP,
		AcceptFunc acceptf = NULL,
		RecvFunc recvf = NULL,
		SendFunc sendf = NULL,
		RecvFromFunc recvfromf = NULL,
		SendToFunc sendtof = NULL
	);

	//  ‰»Î
	MServerParamter& operator<<(AcceptFunc func);
	MServerParamter& operator<<(RecvFunc func);
	MServerParamter& operator<<(SendFunc func);
	MServerParamter& operator<<(RecvFromFunc func);
	MServerParamter& operator<<(SendToFunc func);
	MServerParamter& operator<<(std::string& ip);
	MServerParamter& operator<<(short port);
	MServerParamter& operator<<(MTYPE type);
	//  ‰≥ˆ
	MServerParamter& operator>>(AcceptFunc& func);
	MServerParamter& operator>>(RecvFunc& func);
	MServerParamter& operator>>(SendFunc& func);
	MServerParamter& operator>>(RecvFromFunc& func);
	MServerParamter& operator>>(SendToFunc& func);
	MServerParamter& operator>>(std::string& ip);
	MServerParamter& operator>>(short& port);
	MServerParamter& operator>>(MTYPE& type);

	MServerParamter(const MServerParamter& param);
	MServerParamter& operator= (const MServerParamter& param);

	std::string m_ip;
	short m_port;
	MTYPE m_type;
	AcceptFunc m_acceptf;
	RecvFunc m_recvf;
	SendFunc m_sendf;
	RecvFromFunc m_recvfromf;
	SendToFunc m_sendtof ;
};

class MServer : public ThreadFuncBase
{
public:
	MServer(const MServerParamter& paramter);
	~MServer();

	int Invoke(void* arg);
	int Stop(void* arg);

	int Send(MSOCKET& client, MBuffer& buffer);

	int SendTo(MBuffer& buffer, MSockaddrIn& addr);

private:
	int threadFunc();
	int threadTCPFunc();
	int threadUDPFunc();

private:
	MServerParamter m_params;
	void* m_arg;
	CMyThread m_thread;
	MSOCKET m_sock;
	std::atomic<bool> m_status;
	std::map<int, sockaddr_in> map_KeyAddr;
};





