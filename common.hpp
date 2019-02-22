#ifndef __COMMON_HPP__
#define __COMMON_HPP__
#include<stdio.h>
#include<assert.h>
#include<stdarg.h>
#include<iostream>
#include<string>
#include<string.h>
#include<errno.h>
#include<map>
#include<signal.h>
using namespace std;
#include<sys/epoll.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>
#include<netdb.h>
#define __TRACE__
#define __DEBUG__
static string GetFileName(const string& path)
{
	char ch = '/';
#ifdef _WIN32
	ch = '\\';
#endif
	size_t pos = path.rfind(ch);
	if (pos == string::npos)
		return path;
	else
		return path.substr(pos + 1);
}
inline static void __TraceDebug(const char* filename, int line, const char* function, const char* format, ...)
{
#ifdef __TRACE__
	//输出调用函数的信息
	fprintf(stdout, "[TRACE][%s:%d] %s:", GetFileName(filename).c_str(), line, function);
	//输出用户打的trace信息
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	fprintf(stdout, "\n");
#endif
}
inline static void __ErrorDebug(const char* filename, int line, const char* function, const char* format, ...)
{
#ifdef __DEBUG__
	//输出调用函数的信息
	fprintf(stdout, "[ERROR][%s:%d] %s:", GetFileName(filename).c_str(), line, function);
	//输出用户打的trace信息
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	fprintf(stdout, " errmsg:%s, errno:%d\n", strerror(errno), errno);
#endif
}
#define TraceLog(...)\
	__TraceDebug(__FILE__,__LINE__,__FUNCTION__,__VA_ARGS__);
#define ErrorLog(...)\
	__ErrorDebug(__FILE__,__LINE__,__FUNCTION__,__VA_ARGS__);
#endif //__COMMON_HPP__
