#ifndef __HTTPD_SERVER_HTTP__
#define __HTTPD_SERVER_HTTP__
#include<pthread.h>
#include "ProtocolUtil.hpp"
#include "ThreadPool.hpp"
class HttpdServer//创建监听套接字，让服务器起来
{
private:
    int listen_sock;//监听套接字
    int port;//端口号
    ThreadPool* tp;
public:
    HttpdServer(int port_)
        : port(port_)//初始化端口
        , listen_sock(-1)//初始化监听套接字
        , tp(NULL)
    {}
    void InitServer()//初始化服务器
    {
        //创建套接字
        listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        if(listen_sock < 0){
            LOG(ERROR, "Create socket error!");//日志
            exit(2);
        }
        int opt_ = 1;
        //当服务器挂掉，但是端口号还是在使用着，立即重启不可能，因为此时有一个连接正在使用着你想绑定的那个端口号。所以设置为如下：
        //避免了当服务器启动时立即的报错(该端口号已被绑定)
        //当服务器端是主动断开连接的一方，或服务器的listen_sock一旦挂掉，即使进入TIME_WAIT(而我们用户认为连接未断开)服务器也会立即重启
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt_, sizeof(opt_));
        //绑定
        struct sockaddr_in local_;
        local_.sin_family = AF_INET;
        local_.sin_port = htons(port);
        local_.sin_addr.s_addr = INADDR_ANY;
        if(bind(listen_sock, (struct sockaddr*)&local_, sizeof(local_)) < 0){
            LOG(ERROR, "Bind socket error!");
            exit(3);
        }
        //监听
        if(listen(listen_sock, 5) < 0){
            LOG(ERROR, "Listen socket error!");
            exit(4);
        }
        LOG(INFO, "InitServer success!");
        tp = new ThreadPool();
        tp->initThreadPool();
    }
    //服务器启动
    void Start()
    {
        LOG(INFO, "The server start!");
        for( ; ; ){
            //服务器一旦启动，就要一直处于接收请求(获得连接)的状态
            struct sockaddr_in peer_;
            socklen_t len_ = sizeof(peer_);
            int sock_ = accept(listen_sock, (struct sockaddr*)&peer_, &len_);
            if(sock_ < 0){
                LOG(WARNING, "Accept socket error!");
                continue;
            }
            Task t;
            t.SetTask(sock_, Entry::RequestHandler);
            tp->PushTask(t);
        }
    }
    ~HttpdServer()
    {
        if(listen_sock != -1){
            close(listen_sock);
        }
        port= -1;
    }
};
#endif
