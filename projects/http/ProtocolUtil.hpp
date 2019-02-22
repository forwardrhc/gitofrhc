#ifndef __PROTOCOL_UTIL_HPP__
#define __PROTOCOL_UTIL_HPP__
#include<iostream>
#include<string>
#include<string.h>
#include<strings.h>
#include<sstream>
#include<unordered_map>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<sys/sendfile.h>
#include<sys/wait.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include "Log.hpp"

#define OK 200
#define NOT_FOUND 404
#define BAD_REQUEST 400
#define SERVER_ERROR 500

#define WEB_ROOT "webroot"//web根目录
#define HOME_PAGE "index.html"//首页
#define PAGE_404 "404.html"
#define HTTP_VERSION "http/1.0"
//————————————————————————————————协议工具—————————————————————————————————————
std::unordered_map<std::string, std::string> suffix_map{
    {".html", "text/html"},
    {".htl", "text/html"},
    {".css", "text/css"},
    {".js", "text/application/x-javascript"}
};
class ProtocolUtil
{
public:
    //制作键值对
    static void MakeKV(std::unordered_map<std::string, std::string> &kv_, std::string &str_)
    {
        std::size_t pos = str_.find(": ");
        if(std::string::npos == pos){
            return;
        }
        //提取K串
        std::string k_ = str_.substr(0, pos);
        //提取V串
        std::string v_ = str_.substr(pos+2);
        kv_.insert(make_pair(k_, v_));
    }
    static std::string IntToString(int code)//将整型的错误码转换为字符串
    {
        std::stringstream ss;
        ss << code;
        return ss.str();
    }
    static std::string CodeToDescription(int code)//错误码转换为错误码描述
    {
        switch(code){
            case 200:
                return "OK";
            case 400:
                return "Bad_Request";
            case 404:
                return "Not_Found";
            case 500:
                return "Internal Server Error";
            default:
                return "Unknow";     
        }
    }
    static std::string SuffixToType(const std::string &suffix_)//资源后缀转换为其相应的资源类型
    {
        return suffix_map[suffix_];
    }
};
//——————————————————————————————————请求————————————————————————————————————
class Request
{ 
public:
    Request()
        : blank("\n")//请求空行初始化
        , cgi(false)//初始化cgi：默认不采用cgi技术
        , path(WEB_ROOT)//初始化资源路径，后面会在其后加上文件路径
        , resource_size(0)//初始化文件资源大小
        , content_length(-1)//初始化请求正文长度
        , suffix(".html")
    {}
    void RequestLineParse()//请求首行解析
    {
        //GET /a/b/c.html HTTP/1.1
        std::stringstream ss(rq_line);
        ss >> method >> uri >> version;
    }
    bool IsValidMethod()//判定请求方法是否合法
    {
        //忽略大小写比较字符串
        if(strcasecmp(method.c_str(), "GET") == 0 ||\
           (cgi = (strcasecmp(method.c_str(), "POST") == 0))){
            return true;
        }
        return false;
    }
    void UriParse()//解析URI
    {
        if(strcasecmp(method.c_str(), "GET") == 0){
            std::size_t pos_ = uri.find('?');
            //如果有'?'，即在URL中传参
            ////则证明使用的是GET方法，执行模式必定为cgi
            if(std::string::npos != pos_){
                cgi = true;
                //分离URI
                path += uri.substr(0, pos_);//URI前一部分：资源路径
                param = uri.substr(pos_ + 1);//URI后一部分：参数
            }
            else{
                path += uri;
            }
        }
        else{
            //无'?'，则是POST方法，此时URI就是资源
            path += uri;
        }
        if(path[path.size() -1] == '/'){
            //如果资源路径结尾是'/'，给它加上一个HOME_PAGE
            path += HOME_PAGE;
        }
    }
    bool IsValidPath()//判定请求资源路径是否合法
    {
        struct stat st;
        //资源(文件)不存在
        if(stat(path.c_str(), &st) < 0){
            LOG(WARNING, "The path is not found!");
            return false;
        }
        //资源(文件)存在
        //判断该文件是否为目录
        if(S_ISDIR(st.st_mode)){
            //目录
            path += "/";
            path += HOME_PAGE;
        }
        //普通文件
        //再检测是否为二进制可执行文件
        else{
            //只要其他用户或所属组或拥有者其一对该文件具有可执行权限，即为可执行文件
            if((st.st_mode & S_IXUSR) ||\
               (st.st_mode & S_IXGRP) ||\
               (st.st_mode & S_IXOTH)){
                   //以CGI方式运行
                   cgi = true;
            }
        }
        resource_size = st.st_size;
        std::size_t pos = path.rfind(".");
        if(std::string::npos != pos){
            suffix = path.substr(pos);
        }
        return true;
    }
    //请求报头解析
    bool RequestHeadParse()
    {
        int start = 0;
        while(start < rq_head.size()){
            std::size_t pos = rq_head.find('\n', start);
            if(std::string::npos == pos){
                break;
            }
            std::string sub_string_ = rq_head.substr(start, pos - start);
            if(!sub_string_.empty()){
                LOG(INFO, "Substr is not empty!");
                //制作键值对
                ProtocolUtil::MakeKV(head_kv, sub_string_);
            }
            else{
                LOG(INFO, "Substr is empty!");
                break;
            }
            start = pos + 1;
        }
        return true;
    }
    bool IsNeedRecvText()//判断是否需要读取正文
    {
        if(strcasecmp(method.c_str(), "POST") == 0){
            //若使用POST方法，则要读取正文
            return true;
        }
        return false;
    }
    int GetContentLength()//获取请求正文长度
    {
        std::string cl_ = head_kv["Content-Length"];
        if(!cl_.empty()){
            std::stringstream ss(cl_);
            ss >> content_length;
        }
        return content_length;
    }
    std::string &GetPath()
    {
        return path;
    }
    void SetPath(std::string& path_)
    {
        path = path_;
    }
    int GetResourceSize()
    {
        return resource_size;
    }
    void SetResourceSize(int rs_)
    {
        resource_size = rs_;
    }
    std::string &GetSuffix()
    {
        return suffix;
    }
    void SetSuffix(std::string suffix_)
    {
        suffix = suffix_;
    }
    std::string &GetParam()
    {
        return param;
    }
    bool IsCgi()
    {
        return cgi;
    }
    ~Request()
    {}    
public:
    //请求报文结构
    std::string rq_line;//请求首行
    std::string rq_head;//请求报头
    std::string blank;//空行
    std::string rq_text;//请求正文
private:
    //请求首行内容
    std::string method;//请求方法
    std::string uri;//请求URL(URI)
    std::string version;//协议版本
    bool cgi;//CGI技术——method==POST,GET->uri(?)
    //URI结构
    std::string path;//所要访问资源的路径
    std::string param;//参数

    int resource_size;//资源(文件)大小
    std::string suffix;//资源后缀
    int content_length;//请求正文长度
    std::unordered_map<std::string, std::string> head_kv;//报头中的键值对
};
//————————————————————————————————响应——————————————————————————————————————
class Response
{ 
public:
    Response()
        : blank("\n")//响应空行初始化
        , code(OK)
        , fd(-1)
    {}
    void MakeStatusLine()
    {
        rsp_line = HTTP_VERSION;
        rsp_line += " ";
        rsp_line += ProtocolUtil::IntToString(code);
        rsp_line += " ";
        rsp_line += ProtocolUtil::CodeToDescription(code);
        rsp_line += "\n";
    }
    void MakeResponseHead(Request* &rq_)
    {
        rsp_head = "Content_Length";
        rsp_head += ProtocolUtil::IntToString(rq_->GetResourceSize());
        rsp_head += "\n";
        rsp_head += "Content-Type: ";
        std::string suffix_ = rq_->GetSuffix();
        rsp_head += ProtocolUtil::SuffixToType(suffix_);
        rsp_head += "\n";
    }
    void OpenResource(Request* &rq_)
    {
        std::string path_= rq_->GetPath();
        fd = open(path_.c_str(), O_RDONLY);
    }
    ~Response()
    {
        if(fd >= 0){
            close(fd);
        }
    }    
public:
    std::string rsp_line;//响应首行
    std::string rsp_head;//响应报头
    std::string blank;//空行
    std::string rsp_text;//响应正文
    int fd;
public:
    int code;//错误码
};
//————————————————————————————————连接——————————————————————————————————————
class Connect
{
public:
    Connect(int sock_)
        : sock(sock_)
    {}
    int RecvOneLine(std::string &line_)//读一行，并且在每一行以'\n'结尾
    {
        char c = 'X';
        while(c != '\n'){
            ssize_t s = recv(sock, &c, 1, 0);//阻塞式地读
            if(s > 0){
                if(c == '\r'){
                    recv(sock, &c, 1, MSG_PEEK);//再读一次，但是只是看看'\r'后面是什么字符，并不拿走
                    if(c == '\n'){
                        //此时再读一次并拿走，而且把c由'\r'变成'\n'
                        recv(sock, &c, 1, 0);//此时已将"\r\n"转成了'\n'
                    }
                    //若'\r'后不是'\n'
                    else{
                        //直接转成'\n'
                        c = '\n';
                    }
                }
                line_.push_back(c);
            }
            else{
                break;
            }
        }
        return line_.size();
    }
    void RecvRequestHead(std::string &head_)//读请求报头
    {
        head_ = "";
        std::string line_;
        while(line_ != "\n"){
            line_ = "";
            RecvOneLine(line_);
            head_ += line_;
        }
    }
    void RecvRequestText(std::string text_, int len_, std::string &param_)//读请求正文
    {
        char c_;
        int i = 0;
        //此时已经可以读正文，即是POST方法
        while(i < len_){
            recv(sock, &c_, 1, 0);
            text_.push_back(c_);
            i++;
        }
        param_ = text_;
    }
    void SendResponse(Response* &rsp_, Request* &rq_, bool cgi_)
    {
        std::string& rsp_line_ = rsp_->rsp_line;
        std::string& rsp_head_ = rsp_->rsp_head;
        std::string& blank_ = rsp_->blank;
        send(sock, rsp_line_.c_str(), rsp_line_.size(), 0);
        send(sock, rsp_head_.c_str(), rsp_head_.size(), 0);
        send(sock, blank_.c_str(), blank_.size(), 0);
        if(cgi_){
            std::string& rsp_text_ = rsp_->rsp_text;
            send(sock, rsp_text_.c_str(), rsp_text_.size(), 0);
        }
        else{
            int& fd = rsp_->fd;
            sendfile(sock, fd, NULL, rq_->GetResourceSize());
        }
    }
    ~Connect()
    {
        //合法的sock
        if(sock >= 0){
            close(sock);
        }
    }
private:
    int sock;
};
//—————————————————————————————————入口Entry——————————————————————————————————
//所有的HTTP从此处开始（进入）
class Entry
{
public:
    static void ProcessNonCgi(Connect* &conn_, Request* &rq_, Response* &rsp_)
    {
        int& code_ = rsp_->code;//错误码
        rsp_->MakeStatusLine();//构建状态行
        rsp_->MakeResponseHead(rq_);//构建响应报头
        rsp_->OpenResource(rq_);
        conn_->SendResponse(rsp_, rq_, false);
    }
    static void ProcessCgi(Connect* &conn_, Request* &rq_, Response* &rsp_)
    {
        int& code_ = rsp_->code;
        int input[2];
        int output[2];
        std::string& param_ = rq_->GetParam();
        std::string& rsp_text_ = rsp_->rsp_text;
        pipe(input);
        pipe(output);
        pid_t id = fork();
        if(id < 0){
            code_ = SERVER_ERROR;
            return;
        }
        else if(id == 0){//子进程
            close(input[1]);
            close(output[0]);
            const std::string& path_ = rq_->GetPath();
            std::string cl_env_ = "Content_Length=";
            cl_env_ += ProtocolUtil::IntToString(param_.size());
            putenv((char*)cl_env_.c_str());
            dup2(input[0], 0);
            dup2(output[1], 1);
            execl(path_.c_str(), path_.c_str(), NULL);
            exit(1);
        }
        else{//父进程
            close(input[0]);
            close(output[1]);
            size_t size_ = param_.size();
            size_t total_ = 0;
            size_t cur_ = 0;
            const char* p_ = param_.c_str();
            while(total_ < size_ &&\
                (cur_ = write(input[1], p_ + total_, size_ - total_)) > 0){
                    total_ += cur_; 
                }
            }
        char c;
        while(read(output[0], &c, 1) > 0){
            rsp_text_.push_back(c);
        }
        waitpid(id, NULL, 0);
        close(input[1]);
        close(output[0]);
        rsp_->MakeStatusLine();
        rq_->SetResourceSize(rsp_text_.size());
        rsp_->MakeResponseHead(rq_);
        conn_->SendResponse(rsp_, rq_, true);
    }
    //执行响应
    static int ProcessResponse(Connect* &conn_, Request* &rq_, Response* &rsp_)
    {
        if(rq_->IsCgi()){
            ProcessCgi(conn_, rq_, rsp_);
        }
        else{
            ProcessNonCgi(conn_, rq_, rsp_);
        }
    }
    static void Process404(Connect* &conn_, Request* &rq_, Response* &rsp_)
    {
        std::string path_ = WEB_ROOT;
        path_ += "/";
        path_ += PAGE_404;
        struct stat st;
        stat(path_.c_str(), &st);
        rq_->SetResourceSize(st.st_size);
        rq_->SetSuffix(".html");
        rq_->SetPath(path_);
        ProcessNonCgi(conn_, rq_, rsp_);
    }
    static void HandlerError(Connect* &conn_, Request* &rq_, Response* &rsp_)
    {
        int& code_ = rsp_->code;
        switch(code_){
            case 400:
                break;
            case 404:
                Process404(conn_, rq_, rsp_);
                break;
            case 500:
                break;
            case 503:
                break;
        }
    }
    //处理HTTP请求
    static int RequestHandler(int sock_)
    {
        Connect* conn_ = new Connect(sock_);
        Request* rq_ = new Request();
        Response* rsp_ = new Response();
        int& code_ = rsp_->code;
        conn_->RecvOneLine(rq_->rq_line);
        //请求首行解析
        rq_->RequestLineParse();
        //如果请求方法不合法
        if(!rq_->IsValidMethod()){
            conn_->RecvRequestHead(rq_->rq_head);
            code_ = BAD_REQUEST;
            goto end;
        }
        rq_->UriParse();//URI解析
        //检测资源路径合法性
        if(!rq_->IsValidPath()){
            conn_->RecvRequestHead(rq_->rq_head);
            code_ = NOT_FOUND;
            goto end;
        }
        //合法
        LOG(INFO, "Request path is existing!");
        //读请求报头
        conn_->RecvRequestHead(rq_->rq_head);
        //请求报头解析
        if(rq_->RequestHeadParse()){
            LOG(INFO, "Parse head success!");
        }
        else{
            code_ = BAD_REQUEST;
            goto end;
        }
        //判断是否需要读取正文
        if(rq_->IsNeedRecvText()){
            conn_->RecvRequestText(rq_->rq_text, rq_->GetContentLength(), rq_->GetParam());
        }
        //请求已经读完
        //开始执行响应
        ProcessResponse(conn_, rq_, rsp_);
end:
        if(code_ != OK){
            //处理异常
            HandlerError(conn_, rq_, rsp_);
        }
        delete conn_;
        delete rq_;
        delete rsp_;
        return code_;
    }
};
#endif
