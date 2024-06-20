#ifndef __WEB_BENCH_H__
#define __WEB_BENCH_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <pcre.h>
#include <netdb.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

extern char *method;
extern char *httpHead;
extern char *httpData;
extern char *url;

typedef struct {
    char *protocol;
    char *host;
    int port;
    char *path;
    char *param;
    char *anchor;
} Url;

#define URL_INIT {NULL, NULL, 0, NULL, NULL, NULL}


// 解析参数
int parseParam(int argc, char *const *argv);

// 解析url中的信息
int parseUrl(char *url, Url *U);
void freeUrl(Url *u);

// 构建请求
char *buildRequest(Url *u);

// 进行压测
void bench(char *request, Url *url);

void clean();

#endif //  __WEB_BENCH_H__
