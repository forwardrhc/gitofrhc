#include "HttpdServer.hpp"
#include<signal.h>
#include<unistd.h>
static void Usage(std::string proc_)
{
    std::cout << "Usage " << proc_ << "  port" << std::endl;
}
//./HttpdServer 该服务器启动时所要绑定的端口号
int main(int argc, char* argv[])
{
    if(argc != 2){
        //参数个数不等于2，说明不会用
        Usage(argv[0]);//输入的可执行程序的名称
        exit(1);//直接终止,"1"是程序的退出码
    }
    signal(SIGPIPE, SIG_IGN);
    HttpdServer* serp = new HttpdServer(atoi(argv[1]));
    serp->InitServer();
    daemon(1, 0);
    serp->Start();
    delete serp;
    return 0;
}
