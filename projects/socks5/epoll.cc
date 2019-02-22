#include "epoll.hpp"
void EpollServer::Start()
{
	_listenfd = socket(PF_INET, SOCK_STREAM, 0);
	if (_listenfd == -1)
	{
		ErrorLog("Create Socket!");
		return;
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	//监听本机的所有网卡
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(_listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		ErrorLog("Bind Socket!");
		return;
	}
	if (listen(_listenfd, 100000) < 0)
	{
		ErrorLog("Listen Socket!");
		return;
	}
	TraceLog("Epoll Server Listen On %d", _port);
	_eventfd = epoll_create(100000);
	if (_eventfd == -1)
	{
		ErrorLog("Epoll Create!");
		return;
	}
	//添加listenfd到epoll，监听连接事件
	SetNonblocking(_listenfd);
    OpEvent(_listenfd, EPOLLIN, EPOLL_CTL_ADD);//EPOLLIN:读事件
	//进入事件循环
	EventLoop();
}
void EpollServer::EventLoop()
{
	struct epoll_event events[100000];
	while (1)
	{
		int eveNum = epoll_wait(_eventfd, events, 100000, 0);
		for (int i = 0; i < eveNum; ++i)
		{
			if (events[i].data.fd == _listenfd)
			{
				struct sockaddr* clientaddr;
				socklen_t len;
				int connectfd = accept(_listenfd, clientaddr, &len);
				if (connectfd < 0)
					ErrorLog("Accept Listenfd!");
				ConnectEventHandle(connectfd);
			}
			else if (events[i].events & EPOLLIN)//读事件
			{
				ReadEventHandle(events[i].data.fd);
			}
			else if (events[i].events & EPOLLOUT)//写事件
			{
				WriteEventHandle(events[i].data.fd);
			}
			else
			{
				ErrorLog("event: %d", events[i].data.fd);
			}
		}
	}
}
void EpollServer::RemoveConnect(int fd)
{
	OpEvent(fd, 0, EPOLL_CTL_DEL);
	map<int, Connect*>::iterator it = _fdConnectMap.find(fd);
	if (it != _fdConnectMap.end())
	{
		Connect* conn = it->second;
		if (--conn->_ref == 0)
		{
			delete conn;
			_fdConnectMap.erase(it);
		}
	}
	else
	{
		assert(false);
	}
}
//-------------------------------------------------------通道数据的转发-------------------------------------------------------------------------
//接到数据之后进行通道转发
void EpollServer::Forwarding(Channel* clientChannel, Channel* serverChannel, bool sendencry, bool recvdecrypt)
{
	char buf[4096];
	int rlen = recv(clientChannel->_fd, buf, 4096, 0);
	if (rlen < 0)
	{
		ErrorLog("recv : %d", clientChannel->_fd);
	}
	else if (rlen == 0)//单向关闭
	{
		//client发起FIN，client channel不会再发数据了，但是还不能关serverChannel，因为还有可能回复
		shutdown(serverChannel->_fd, SHUT_WR);//所以我也不会再向server发数据了，shutdown它的write
		RemoveConnect(clientChannel->_fd);
	}
	else
	{
		if (recvdecrypt)
		{
			Decrypt(buf, rlen);
		}
		if (sendencry)
		{
			Encry(buf, rlen);
		}
		//收到数据发送
		buf[rlen] = '\0';
		SendInLoop(serverChannel->_fd, buf, rlen);
	}
}
void EpollServer::SendInLoop(int fd, const char* buf, int len)
{
	int slen = send(fd, buf, len, 0);
	if (slen < 0)
	{
		ErrorLog("Send to %d", fd);
	}
	else if (slen < len)
	{
		TraceLog("recv %d bytes, send %d bytes, left %d send in loop", len, slen, len - slen);
		map<int, Connect*>::iterator it = _fdConnectMap.find(fd);
		if (it != _fdConnectMap.end())
		{
			Connect* conn = it->second;
			Channel* channel = &conn->_clientChannel;
			if (fd == conn->_serverChannel._fd)
				channel = &conn->_serverChannel;
			int events = EPOLLOUT | EPOLLIN | EPOLLONESHOT;//添加一次写事件
			OpEvent(fd, events, EPOLL_CTL_MOD);
			channel->_buf.append(buf + slen);
		}
		else
		{
			assert(false);
		}
	}
}
//----------------------------------------------------------------------------------------------------------------------------------------------
void EpollServer::WriteEventHandle(int fd)
{
	map<int, Connect*>::iterator it = _fdConnectMap.find(fd);
	if (it != _fdConnectMap.end())
	{
		Connect* conn = it->second;
		Channel* channel = &conn->_clientChannel;
		if (fd == conn->_serverChannel._fd)
		{
			channel = &conn->_serverChannel;
		}
		string buf;
		buf.swap(channel->_buf);
		SendInLoop(fd, buf.c_str(), buf.size());
	}
	else
	{
		assert(fd);
	}
}
