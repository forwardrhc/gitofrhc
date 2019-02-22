#include "socks5.hpp"
void Socks5Server::ConnectEventHandle(int connectfd)
{
	TraceLog("New Connect Event:%d", connectfd);
	//添加connectfd到epoll，监听读事件
	SetNonblocking(connectfd);//设置为非阻塞的连接
	OpEvent(connectfd, EPOLLIN, EPOLL_CTL_ADD);
	Connect* conn = new Connect;
	conn->_state = AUTH;
	conn->_clientChannel._fd = connectfd;
	_fdConnectMap[connectfd] = conn;
	conn->_ref++;
}
//0：数据没到，继续等待
//1：成功
//-1：失败
int Socks5Server::AuthHandle(int fd)//身份认证的授权处理
{
	char buf[260];
	int rlen = recv(fd, buf, 260, MSG_PEEK);
	if (rlen <= 0 )
	{
		return -1;
	}
	else if (rlen < 3)
	{
		return 0;
	}
	else
	{
		recv(fd, buf, rlen, 0);
		Decrypt(buf, rlen);
		if (buf[0] != 0x05)
		{
			ErrorLog("Not Socks5!");
			return -1;
		}
		return 1;
	}
}
//-2：数据没到
//serverfd：连接成功
//-1：失败
int Socks5Server::EstablishedHandle(int fd)
{
	char buf[256];
	int rlen = recv(fd, buf, 256, MSG_PEEK);
	if (rlen <= 0)
	{
		return -1;
	}
	else if (rlen < 10)
	{
		return -2;
	}
	else
	{
		char ip[4];//IPv4——4字节
		char port[2];
		recv(fd, buf, 4, 0);
		Decrypt(buf, 4);
		char addresstype = buf[3];
		if (addresstype == 0x01)//IPv4
		{
			recv(fd, ip, 4, 0);
			Decrypt(ip, 4);
			recv(fd, port, 2, 0);
			Decrypt(port, 2);
		}
		else if (addresstype == 0x03)//域名
		{
			char len = 0;
			//recv域名
			recv(fd, &len, 1, 0);
			Decrypt(&len, 1);
			recv(fd, buf, len, 0);
			buf[len] = '\0';
			TraceLog("Encry Domainname:%s", buf);
			Decrypt(buf, len);
			//recv port
			recv(fd, port, 2, 0);
			Decrypt(port, 2);
			buf[len] = '\0';
			TraceLog("Decrypt Domainname:%s", buf);
			//获取IP
			struct hostent* hostptr = gethostbyname(buf);
			memcpy(ip, hostptr->h_addr, hostptr->h_length);
		}
		else if (addresstype == 0x04)//IPv6——16字节
		{
			ErrorLog("Not Support IPv6!");
			return -1;
		}
		else
		{
			ErrorLog("Invalid Address Type!");
			return -1;
		}
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family = AF_INET;
		memcpy(&addr.sin_addr.s_addr, ip, 4);
		addr.sin_port = *((uint16_t*)port);
		int serverfd = socket(AF_INET, SOCK_STREAM, 0);
		if (serverfd < 0)
		{
			ErrorLog("Server Socket!");
			return -1;
		}
		if (connect(serverfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		{
			ErrorLog("Connect Error!");
			close(serverfd);
			return -1;
		}
		return serverfd;
	}
}
void Socks5Server::ReadEventHandle(int connectfd)
{
	map<int, Connect*>::iterator it = _fdConnectMap.find(connectfd);
	if (it != _fdConnectMap.end())
	{
		Connect* conn = it->second;
		if (conn->_state == AUTH)
		{
			char reply[2];
			reply[0] = 0x05;
			int ret = AuthHandle(connectfd);
			if (ret == 0)
			{
				return;
			}
			else if (ret == 1)
			{
				reply[1] = 0x00;
				conn->_state = ESTABLISHED;
			}
			else if (ret == -1)
			{
				reply[0] = 0xFF;
				RemoveConnect(connectfd);
			}
			Encry(reply, 2);
			if (send(connectfd, reply, 2, 0) != 2)
			{
				ErrorLog("Auth Reply!");
			}
		}
		else if (conn->_state == ESTABLISHED)
		{
			//回复
			char reply[10] = { 0 };
			reply[0] = 0x05;

			int serverfd = EstablishedHandle(connectfd);
			if (serverfd == -1)
			{
				reply[1] = 0x01;
				RemoveConnect(connectfd);
			}
			else if (serverfd == -2)
			{
				return;
			}
			else
			{
				reply[1] = 0x00;
				reply[3] = 0x01;
			}
			Encry(reply, 10);
			if (send(connectfd, reply, 10, 0) != 10)
			{
				ErrorLog("Established Reply!");
			}
			if (serverfd >= 0)
			{
				SetNonblocking(serverfd);
				OpEvent(serverfd, EPOLLIN, EPOLL_CTL_ADD);
				conn->_serverChannel._fd = serverfd;
				_fdConnectMap[serverfd] = conn;
				conn->_ref++;
				conn->_state = FORWARDING;
			}
		}
		else if (conn->_state == FORWARDING)
		{
			Channel* clientChannel = &conn->_clientChannel;
			Channel* serverChannel = &conn->_serverChannel;
			bool sendencry = false;
			bool recvdecrypt = true;
			if (connectfd == serverChannel->_fd)
			{
				swap(clientChannel, serverChannel);
				swap(sendencry, recvdecrypt);
			}
			//client -> server
			Forwarding(clientChannel, serverChannel, sendencry, recvdecrypt);
		}
		else
		{
			assert(false);
		}
	}
	else
	{
		assert(false);
	}
}
int main()
{
	Socks5Server server(10000);
	server.Start();
}
