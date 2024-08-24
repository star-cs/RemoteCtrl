#include "pch.h"
#include "MNetwork.h"
#include <conio.h>
#include <list>
#include "Tool.h"
#include "Packet.h"
#include "ServerSocket.h"

MServer::MServer(const MServerParamter& paramter) : m_status(true), m_arg(NULL)
{
    m_params = paramter;
    m_thread.UpdateWorker(ThreadWorker(this, (FUNCTYPE)&MServer::threadFunc));
}

MServer::~MServer()
{
	m_thread.Stop();
}

int MServer::Invoke(void* arg)
{
    m_sock.reset(new MSocket(m_params.m_type));
    if (*m_sock == INVALID_SOCKET) {
        printf("%s(%d):%s ERROR(%d)\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        return -1;
    }

    if (m_params.m_type == MTYPE::MTypeTCP) {
        if (m_sock->listen() == -1) {
            return -2;
        }
    }

    MSockaddrIn client_addr;

    if (-1 == m_sock->bind(m_params.m_ip, m_params.m_port)) {
        printf("%s(%d):%s ERROR(%d)\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        return -3;
    }

    if (m_thread.Start() == false) { return -4; }
    m_arg = arg;
    return 0;
}

int MServer::Stop(void* arg)
{
    if (m_status == true) {
        m_sock->close();
        m_status = false;
        m_thread.Stop();
    }
    return 0;
}

int MServer::Send(MSOCKET& client, MBuffer& buffer)
{
    int ret = m_sock->send(buffer);           // TODO: 处理不完整。
    if (m_params.m_sendf) m_params.m_sendf(m_arg, client, ret);
    return ret;
}

int MServer::SendTo(MBuffer& buffer, MSockaddrIn& addr)
{
    int ret = m_sock->sendto(buffer, addr);   // TODO: 处理不完整。
    if (m_params.m_sendf) m_params.m_sendtof(m_arg, addr, ret);
    return ret;
}

int MServer::threadFunc()
{
    if (m_params.m_type == MTYPE::MTypeTCP)
    {
        threadTCPFunc();
    }
    else {
        threadUDPFunc();
    }
    m_thread.Stop();
    return 0;
}

int MServer::threadTCPFunc()
{
    return 0;
}

int MServer::threadUDPFunc()
{
    MBuffer buffer(1024 * 256);
    MSockaddrIn client_addr;
    int ret = 0;

    while (m_status) {
        ret = m_sock->recvfrom(buffer, client_addr);

        if (ret > 0) {
            CTool::Dump((BYTE*)buffer, ret);

            if (m_params.m_recvfromf != NULL) {
                //m_params.m_recvfromf(m_arg, buffer, client_addr);
            }
            printf("%s(%d):%s client_addr-ip %s client_addr-port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client_addr.GetIP().c_str() , client_addr.GetPort());

            size_t len = ret;
            CPacket pack = CPacket((BYTE*)buffer, len);

            // 类型转换真麻烦 :)
            UdpHole* u = (UdpHole*)pack.strData.c_str();

            printf("%d %d\r\n", u->nType, u->nKey);
            // TODO: 添加映射关系 key - addr
            
            switch (u->nType)
            {
            case 1:
            {
                auto it = map_KeyAddr.insert(std::pair<int, sockaddr_in>(u->nKey, client_addr));
                if (it.second) {
                    printf("%s(%d):%s insert done!\r\n", __FILE__, __LINE__, __FUNCTION__);
                }

                ret = m_sock->sendto(pack, client_addr);
                break;
            }
            case 2:
            {
                auto it = map_KeyAddr.find(u->nKey);
                if (it != map_KeyAddr.end()) {
                    printf("%s(%d):%s find addr done!\r\n", __FILE__, __LINE__, __FUNCTION__);

                    //找到了相应的地址。
                    CPacket pack2 = CPacket(2025, reinterpret_cast<const BYTE*>(&it->second), sizeof(sockaddr_in));

                    ret = m_sock->sendto(pack2, client_addr);
                }
                break;
            }
            }

            
            printf("%s(%d):%s ERROR(%d) ret = %d \r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);

        }
        else {
            printf("%s(%d):%s ERROR(%d) ret = %d \r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
        }
    }
    if (m_status == true) m_status = false;
    m_sock->close();
    printf("%s(%d):%s \r\n", __FILE__, __LINE__, __FUNCTION__);
    return 0;
}

MServerParamter::MServerParamter(const std::string& ip, short port, MTYPE type, AcceptFunc acceptf, RecvFunc recvf, SendFunc sendf, RecvFromFunc recvfromf, SendToFunc sendtof)
    : m_ip(ip)
{
    m_port = port;
    m_type = type;
    m_acceptf = acceptf;
    m_recvf = recvf;
    m_sendf = sendf;
    m_recvfromf = recvfromf;
    m_sendtof = sendtof;
}

MServerParamter& MServerParamter::operator<<(AcceptFunc func)
{
    // TODO: 在此处插入 return 语句
    m_acceptf = func;
    return *this;
}

MServerParamter& MServerParamter::operator<<(RecvFunc func)
{
    // TODO: 在此处插入 return 语句
    m_recvf = func;
    return *this;
}

MServerParamter& MServerParamter::operator<<(SendFunc func)
{
    // TODO: 在此处插入 return 语句
    m_sendf = func;
    return *this;
}

MServerParamter& MServerParamter::operator<<(RecvFromFunc func)
{
    // TODO: 在此处插入 return 语句
    m_recvfromf = func;
    return *this;
}

MServerParamter& MServerParamter::operator<<(SendToFunc func)
{
    // TODO: 在此处插入 return 语句
    m_sendtof = func;
    return *this;
}

MServerParamter& MServerParamter::operator<<(std::string& ip)
{
    // TODO: 在此处插入 return 语句
    m_ip = ip;
    return *this;
}

MServerParamter& MServerParamter::operator<<(short port)
{
    // TODO: 在此处插入 return 语句
    m_port = port;
    return *this;
}

MServerParamter& MServerParamter::operator<<(MTYPE type)
{
    // TODO: 在此处插入 return 语句
    m_type = type;
    return *this;
}

MServerParamter& MServerParamter::operator>>(AcceptFunc& func)
{
    // TODO: 在此处插入 return 语句
    func = m_acceptf;
    return *this;
}

MServerParamter& MServerParamter::operator>>(RecvFunc& func)
{
    // TODO: 在此处插入 return 语句
    func = m_recvf;
    return *this;
}

MServerParamter& MServerParamter::operator>>(SendFunc& func)
{
    // TODO: 在此处插入 return 语句
    func = m_sendf;
    return *this;
}

MServerParamter& MServerParamter::operator>>(RecvFromFunc& func)
{
    // TODO: 在此处插入 return 语句
    func = m_recvfromf;
    return *this;
}

MServerParamter& MServerParamter::operator>>(SendToFunc& func)
{
    // TODO: 在此处插入 return 语句
    func = m_sendtof;
    return *this;
}

MServerParamter& MServerParamter::operator>>(std::string& ip)
{
    // TODO: 在此处插入 return 语句
    ip = m_ip;
    return *this;
}

MServerParamter& MServerParamter::operator>>(short& port)
{
    // TODO: 在此处插入 return 语句
    port = m_port;
    return *this;
}

MServerParamter& MServerParamter::operator>>(MTYPE& type)
{
    // TODO: 在此处插入 return 语句
    type = m_type;
    return *this;
}


MServerParamter::MServerParamter(const MServerParamter& param)
    : m_ip(param.m_ip)
{
    m_port = param.m_port;
    m_type = param.m_type;
    m_acceptf = param.m_acceptf;
    m_recvf = param.m_recvf;
    m_sendf = param.m_sendf;
    m_recvfromf = param.m_recvfromf;
    m_sendtof = param.m_sendtof;
}

MServerParamter& MServerParamter::operator=(const MServerParamter& param)
{
    // TODO: 在此处插入 return 语句
    if (this != &param) {
        m_ip = param.m_ip;
        m_port = param.m_port;
        m_type = param.m_type;
        m_acceptf = param.m_acceptf;
        m_recvf = param.m_recvf;
        m_sendf = param.m_sendf;
        m_recvfromf = param.m_recvfromf;
        m_sendtof = param.m_sendtof;
    }
    return *this;
}
