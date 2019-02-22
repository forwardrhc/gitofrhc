#include "transfer.hpp"
//多态实现的虚函数
void TransferServer::ConnectEventHandle(int connectfd)
{
	int serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverfd == -1)
	{
		ErrorLog("socket!");
		return;
	}
	if (connect(serverfd, (struct sockaddr*)&_socks5addr, sizeof(_socks5addr)) < 0)
	{
		ErrorLog("Connect socks5!");
		return;
	}
	//设置为非阻塞，并添加到epoll
	SetNonblocking(connectfd);
	OpEvent(connectfd, EPOLLIN, EPOLL_CTL_ADD);

	SetNonblocking(serverfd);
	OpEvent(serverfd, EPOLLIN, EPOLL_CTL_ADD);

	Connect* conn = new Connect;
	conn->_state = FORWARDING;
	conn->_clientChannel._fd = connectfd;
	conn->_ref++;
	_fdConnectMap[connectfd] = conn;
	conn->_serverChannel._fd = serverfd;
	conn->_ref++;
	_fdConnectMap[serverfd] = conn;
}
void TransferServer::ReadEventHandle(int connectfd)
{
	map<int, Connect*>::iterator it = _fdConnectMap.find(connectfd);
	if (it != _fdConnectMap.end())
	{
		Connect* conn = it->second;
		Channel* clientChannel = &conn->_clientChannel;
		Channel* serverChannel = &conn->_serverChannel;
		bool sendencry = true;
		bool recvdecrypt = false;
		if (connectfd == conn->_serverChannel._fd)
		{
			swap(clientChannel, serverChannel);
			swap(sendencry, recvdecrypt);
		}
		Forwarding(clientChannel, serverChannel, sendencry, recvdecrypt);
	}
	else
	{
		assert(false);
	}
}
int main()
{
	TransferServer server(9999, "129.204.112.224", 10000);
	server.Start();
}
