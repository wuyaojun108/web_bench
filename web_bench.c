#include "web_bench.h"

#define VERSION "0.0.1"

#define HELP_MSG \
    "webbench [option]... URL\n"  \
    "-h|--help     帮助信息\n"  \
    "-V|--version    版本\n"  \
    "-c|--clients <n>   开启几个客户端进行压测，默认1个\n"   \
    "--count <count>   指定每个客户端的循环次数，默认1次\n"  \
    "-X <GET|POST|HEAD|OPTIONS> 指定http请求使用的方法，默认是get\n"   \
    "-H|--head <key:value>  请求头\n"   \
    "--data <data>       请求体\n"   \
    "--reload      发送没有缓存的请求，-Pragma: no-cache \n"

#define GET_STR "GET"
#define POST_STR "POST"
#define HEAD_STR "HEAD"
#define OPTIONS_STR "OPTIONS"

// 短选项
#define SHORT_OPTS "hVc:X:H:"

// 长选项
static const struct option LONG_OPTS[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {"clients", required_argument, NULL, 'c'},
    {"count", required_argument, NULL, 'C'},
    {"head", required_argument, NULL, 'H'},
    {"data", required_argument, NULL, 'd'},
    {"reload", no_argument, NULL, 'r'},
    {NULL, 0, NULL, 0}  // 结束标志
};

#define OVERCOUNT 128


// 客户端数量
int clients = 1;
// 客户端循环次数
int count = 1;
// http版本
char *httpVersion = "HTTP/1.1";
// 请求方法
char *method = GET_STR;
// 请求头
char *httpHead = NULL;
// 请求体
char *httpData = NULL;
// 是否不缓存请求结果
int reload = 0;
char *url = NULL;

int headLen = 0;
int dataLen = 0;


// 字符串切割
typedef struct {
    char **parts;
    int count;
} StrPart;

#define STR_PART_INIT {NULL, 0}

void appendHead(char *head, int len) {
    if (httpHead == NULL) {
        httpHead = malloc(len + 2); // 多的两个位置是\n\0
    } else {
        httpHead = realloc(httpHead, headLen + len + 2);
    }

    memcpy(&httpHead[headLen], head, len);
    headLen += len;
    httpHead[headLen] = '\n';
    headLen++;
    httpHead[headLen] = '\0';
}


// 大写转小写
char toLower(char c) {
    if (c >= 65 && c <= 90) {
        c += 32;
    }
    return c;
}

// 小写转大写
char toUpper(char c) {
    if (c >= 97 && c <= 122) {
        c -= 32;
    }
    return c;
}

// 比较两个字符串，忽略大小写，相等返回0，否则返回其它
int strcmpIgnoreCase(const char *s1, const char *s2) {
    // 比较长度
    int slen1 = strlen(s1);
    int slen2 = strlen(s2);

    if (slen1 != slen2) {
        return 1;
    }

    // 挨个比较字符
    int i = 0;
    while (i < slen1) {
        char c1 = toUpper(s1[i]);
        char c2 = toUpper(s2[i]);

        if (c1 != c2) {
            return 1;
        }
        i++;
    }
    return 0;
}


int checkUrl(char *url) {
    char *pattern = "^https?://(?:[a-zA-Z0-9-]+\\.)*[a-zA-Z0-9]{1,}(?::\\d{1,5})?(?:/[^\\s]*)?$";

    int errOffset = 0;
    const char *errPtr = NULL;
    int ovector[OVERCOUNT] = {0};

    pcre *re = pcre_compile(pattern, PCRE_EXTENDED, &errPtr, &errOffset, NULL);
    if (re == NULL) {
        printf("编译错误：offset %d, %s\n", errOffset, errPtr);
        return -1;
    }

    int result = pcre_exec(re, NULL, url, strlen(url), 0, 0, ovector, OVERCOUNT);
    pcre_free(re);

    if (result > 0) {
        return 1;
    }
    return -1;
}

int parseUrl(char *url, Url *U) {
    int urlLen = strlen(url);
    char *urlEnd = &url[urlLen];
    int isFinished = 0;

    // 解析协议
    char *protocolEnd = strstr(url, "://");
    U->protocol = malloc(protocolEnd - url + 1);
    memcpy(U->protocol, url, protocolEnd - url);
    U->protocol[protocolEnd - url] = '\0';
    url = protocolEnd + 3;

    // 解析host和端口
    char *portStart = strchr(url, ':');
    char *hostEnd = strchr(url, '/');
    if (portStart == NULL && hostEnd == NULL) {
        // URL中只有host，并且可以确定之后没有其他内容
        hostEnd = urlEnd;
        U->host = malloc(hostEnd - url + 1);
        memcpy(U->host, url, hostEnd - url);
        U->host[hostEnd - url] = '\0';
        isFinished = 1;
    } else if (portStart == NULL || (hostEnd != NULL && hostEnd < portStart)) {
        // url中没有端口信息
        U->host = malloc(hostEnd - url + 1);
        memcpy(U->host, url, hostEnd - url);
        U->host[hostEnd - url] = '\0';
        url = hostEnd;
    } else {
        if (hostEnd == NULL) {
            hostEnd = urlEnd;
        }
        // url中有端口信息
        U->host = malloc(portStart - url + 1);
        memcpy(U->host, url, portStart - url);
        U->host[portStart - url] = '\0';
        url = portStart + 1;

        char *port = malloc(hostEnd - url + 1);
        memcpy(port, url, hostEnd - url);
        port[hostEnd - url] = '\0';
        U->port = atoi(port);
        url = hostEnd;
        free(port);
    }

    if (isFinished) {
        // 如果没有路径，默认路径是 /
        U->path = malloc(2);
        U->path[0] = '/';
        U->path[1] = '\0';
        return 0;
    }
    // 解析路径
    char *pathEnd = strchr(url, '?');
    if (pathEnd == NULL) {
        pathEnd = urlEnd;
        isFinished = 1;
    }
    U->path = malloc(pathEnd - url + 1);
    memcpy(U->path, url, pathEnd - url);
    U->path[pathEnd - url] = '\0';
    url = pathEnd + 1;

    if (isFinished) {
        return 0;
    } 
    // 解析参数
    char *paramEnd = strchr(url, '#');
    if (paramEnd == NULL) {
        paramEnd = urlEnd;
        isFinished = 1;
    }
    U->param = malloc(paramEnd - url + 1);
    memcpy(U->param, url, paramEnd - url);
    U->param[paramEnd - url] = '\0';
    url = paramEnd + 1;

    if (isFinished) {
        return 0;
    } 
    // 解析锚点
    char *anchorEnd = urlEnd;
    U->anchor = malloc(anchorEnd - url + 1);
    memcpy(U->anchor, url, anchorEnd - url);
    U->anchor[anchorEnd - url] = '\0';
    return 0;
}

void freeUrl(Url *u) {
    if (u->protocol != NULL) {
        free(u->protocol);
        u->protocol = NULL;
    }
    if (u->host != NULL) {
        free(u->host);
        u->host = NULL;
    }
    if (u->path != NULL) {
        free(u->path);
        u->path = NULL;
    }
    if (u->param != NULL) {
        free(u->param);
        u->param = NULL;
    }
    if (u->anchor != NULL) {
        free(u->anchor);
        u->anchor = NULL;
    }
}

int parseParam(int argc, char *const *argv) {
    int opt = 0;
    int longOptionIndex = 0;

    opterr = 1; // 打开解析选项时的错误输出
    while ((opt = getopt_long(argc, argv, 
            SHORT_OPTS, LONG_OPTS, &longOptionIndex)) != EOF) {
        switch (opt) {
            case ':':
            case 'h':
                printf("%s\n", HELP_MSG);
                return -1;
            case 'V':
                printf("%s\n", VERSION);
                return -1;
            case 'c':
                clients = atoi(optarg);
                break;
            case 'C':
                count = atoi(optarg);
                break;
            case 'X':
                if (strcmpIgnoreCase(optarg, GET_STR) == 0) {
                    method = GET_STR;
                } else if (strcmpIgnoreCase(optarg, POST_STR) == 0) {
                    method = POST_STR;
                } else if (strcmpIgnoreCase(optarg, HEAD_STR) == 0) {
                    method = HEAD_STR;
                } else if (strcmpIgnoreCase(optarg, OPTIONS_STR) == 0) {
                    method = OPTIONS_STR;
                }
                break;
            case 'H':
                // 请求头，在每行结尾拼接一个换行符
                appendHead(optarg, strlen(optarg));
                break;
            case 'd':
                // 请求体
                httpData = malloc(strlen(optarg) + 1);
                memcpy(httpData, optarg, strlen(optarg));
                httpData[strlen(optarg)] = '\0';
                break;
            case 'r':
                // 是否发送无缓存请求
                reload = 1;
                char *s = "Pragma: no-cache";
                appendHead(s, strlen(s));
                break;
            // 处理选项错误的情况
            case '?':
                return -1;
        }
    }

    if (optind >= argc) {
        printf("缺url\n");
        return -1;
    }

    url = argv[optind];
    if (checkUrl(url) != 1) {
        printf("url格式错误\n");
        return -1;
    }

    // printf("解析结果：clients = %d, method = %s, head = %s, data = %s，url = %s\n"
    //     , clients, method, httpHead, httpData, url);
    return 0;
}

char *buildRequest(Url *u) {
    if (u->port == 0) {
        u->port = 80; // 默认使用80端口
    }

    int requestLen = 1024;
    char request[requestLen];
    memset(request, 0, requestLen);

    // 拼接请求行
    strcat(request, method);
    strcat(request, " ");
    strcat(request, u->path);
    strcat(request, " ");
    strcat(request, httpVersion);
    strcat(request, "\n");

    // 拼接host字段
    char host[128];
    memset(host, 0, 128);
    strcat(host, "Host: ");
    strcat(host, u->host);
    appendHead(host, strlen(host));

    // 拼接UserAgent字段
    char userAgent[128];
    memset(userAgent, 0, 128);
    strcat(userAgent, "User-Agent: ");
    strcat(userAgent, "WebBench ");
    strcat(userAgent, VERSION);
    appendHead(userAgent, strlen(userAgent));

    // 拼接Connection字段
    char *s = "Connection: close";
    appendHead(s, strlen(s));

    if (httpData != NULL) {
        char *s = "Content-Type: text/plain";
        appendHead(s, strlen(s));

        char cLengBuf[128];
        memset(cLengBuf, 0, 128);
        strcat(cLengBuf, "Content-Length: ");
        snprintf(&cLengBuf[strlen(cLengBuf)], 16, "%ld", strlen(httpData));
        appendHead(cLengBuf, strlen(cLengBuf));
    }

    // 请求头
    strcat(request, httpHead);

    // 请求空行
    strcat(request, "\n");

    // 请求体
    if (httpData != NULL) {
        strcat(request, httpData);
        strcat(request, "\n");
    }

    char *req = malloc(strlen(request) + 1);
    memcpy(req, request, strlen(request));
    req[strlen(request)] = '\0';
    return req;
}

/*
  判断一个字符串中是否存储的是数字
*/
int isNumeric(char *str) {
    int sLen = strlen(str);
    int isDecimal = 0;
    int isNegative = 0;

    if (sLen == 0) {
        return 0;
    }

    int i;
    for (i = 0; i < sLen; i++) {
        // 校验1：如果不是数字并且不是“.”、“-”，返回false
        if ((str[i] < 48 || str[i] > 57) && (str[i] != '.' && str[i] != '-')) {
            return 0;
        }
        // 校验2：负号，应该出现在开头、字符串的长度应该大于1、只有一个负号
        if (str[i] == '-') {
            if (i != 0 || sLen == 1 || isNegative) {
                return 0;
            }
            isNegative = 1;
        }
        // 校验3：小数点，应该只有1位、字符串长度应该大于1、不能出现在开头或结尾、只有一个小数点
        if (str[i] == '.') {
            if (sLen == 1 || i == 0 || i == sLen - 1 || isDecimal) {
                return 0;
            }
            isDecimal = 1;
        }
    }
    return 1;
}

int strSplit(char *str, char *s, StrPart *strPart) {
    int strLen = strlen(str);
    int sLen = strlen(s);

    // 提前计算有多少子字符串，提前分配内存
    int count = 0;
    int i = 0;
    while (i < strLen) {
        if (strncmp(&str[i], s, sLen) != 0) {
            i++;
            continue;
        }
        count++;
        i += sLen;
    }
    if (strncmp(&str[strLen - sLen], s, sLen) != 0) {
        count++;
    }
    strPart->parts = malloc(count * sizeof(char *));
    memset(strPart->parts, 0, count * sizeof(char *));

    char *sub = NULL;
    int shouldEnd = 0;
    while (shouldEnd != 1) {
        sub = strstr(str, s);
        if (sub == NULL) {
            char *strEnd = &str[strlen(str)];
            sub = strEnd;   
            shouldEnd = 1;
        }

        char *part = malloc(sub - str + 1);
        memcpy(part, str, sub - str);
        part[sub - str] = '\0';
        strPart->parts[strPart->count] = part;
        
        part = NULL;
        str = sub + sLen;

        strPart->count++;
    }
    return 0;
}

void strPartFree(StrPart *strPart) {
    if (strPart->parts != NULL) {
        for (int i = 0; i < strPart->count; i++) {
            free(strPart->parts[i]);
        }
        free(strPart->parts);
        strPart->parts = NULL;
    }
}

int isIpAddr(char *ip) {
    StrPart strPart = STR_PART_INIT;
    strSplit(ip, ".", &strPart);

    if (strPart.count != 4) {
        strPartFree(&strPart);
        return 0;
    }

    int i;
    for (i = 0; i < strPart.count; i++) {
        char *part = strPart.parts[i];
        if (isNumeric(part) == 0) {
            return 0;
        }
        int num = atoi(part);
        if (part[0] == '0' && strlen(part) > 1) { // 数字有前导0
            return 0;
        }
        if (num < 0 || num > 255) {
            return 0;
        }
    }
    strPartFree(&strPart);
    return 1;
}

char *getIpByHost(char *host) {
    struct hostent *hos = gethostbyname(host);
    if (hos == NULL) {
        return NULL;
    }

    int bufLen = 128;
    char buf[bufLen];
    memset(buf, 0, bufLen);
    int i = 0;
    while (hos->h_addr_list[i] != NULL) {
        const char *ip = inet_ntop(AF_INET, hos->h_addr_list[i], buf, bufLen);
        if (ip != NULL) {
            int len = strlen(buf);
            char *s = malloc(len + 1);
            memcpy(s, ip, len);
            s[len] = '\0';
            return s;
        }
        memset(buf, 0, bufLen);
        i++;
    }
    return NULL;
}

int createAndConnectSock(char *host, int port) {
    // 创建socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("创建socket错误");
        return -1;
    }

    // 配置服务端的地址
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (isIpAddr(host) == 1) {
        addr.sin_addr.s_addr = inet_addr(host);
    } else {
        char *ip = getIpByHost(host);
        if (ip == NULL) {
            printf("根据域名 %s 获取IP地址失败失败\n", host);
            return -1;
        }
        addr.sin_addr.s_addr = inet_addr(ip);
        free(ip);
    }
    
    // 连接服务器
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("连接服务器失败\n");
        return -1;
    }
    return sock;
}

void benchcore(char *req, char *host, int port, 
        int *succeed, int *failed, unsigned long long *writeBytes, 
        unsigned long long *readBytes) {

    int readBufLen = 512000;
    char readBuf[readBufLen];
    memset(readBuf, 0, readBufLen);

    int i;
    for (i = 0; i < count; i++) {
        int sock = createAndConnectSock(host, port);
        if (sock < 0) {
            *failed += 1;
            continue;
        }
        int requestLen = strlen(req);
        if (requestLen != write(sock, req, requestLen)) {
            *failed += 1;
            *writeBytes += requestLen;
            close(sock);
            continue;
        }
        int readLen = read(sock, readBuf, readBufLen);
        if (readLen < 0) {
            *failed += 1;
            *writeBytes += requestLen;
            *readBytes += readLen;
            close(sock);
            continue;
        }

        if (count == 1) {
            printf("如果请求只有一次，会打印响应信息，帮助用户调试：\n");
            printf("%s\n\n", readBuf);
        }

        // 判断响应码
        char *s1 = strchr(readBuf, ' ');
        s1++;
        char *s2 = strchr(s1, ' ');
        int slen = s2 - s1;
        char code[4];
        memcpy(code, s1, slen);
        code[slen] = '\0';

        if (strcmp(code, "200") != 0) {
            *failed += 1;
            *writeBytes += requestLen;
            *readBytes += readLen;
            close(sock);
            continue;
        }

        *writeBytes += requestLen;
        *readBytes += readLen;
        *succeed += 1;
        close(sock);
        memset(readBuf, 0, readBufLen);
    }
}

void bench(char *request, Url *url) {
    printf("请求：\n%s\n", request);
    // 创建socket
    int sock = createAndConnectSock(url->host, url->port);
    if (sock == -1) {
        printf("无法连接服务器，终止压测\n");
        return;
    }

    // 创建管道
    int mypipe[2];
    if (pipe(mypipe) < 0) {
        printf("创建管道失败，终止压测\n");
        return;
    }

    // 创建子进程
    int i;
    for (i = 0; i < clients; i++) {
        int pid = fork();
        if (pid == -1) {
            printf("创建子进程失败，终止压测\n");
            return;
        }
        if (pid == 0) {
            // 子进程

            // 开始时间
            time_t st;
            time(&st);

            int succeed, failed;
            unsigned long long writeBytes, readBytes;
            succeed = failed = writeBytes = readBytes = 0;
            benchcore(request, url->host, url->port, 
                &succeed, &failed, &writeBytes, &readBytes);

            // 结束时间
            time_t et;
            time(&et);

            char resBuf[64];
            memset(resBuf, 0, 64);
            int len = snprintf(resBuf, 64, "%d %d %d %llu %llu\n",
                (int)(et - st), succeed, failed, writeBytes, readBytes);
            close(mypipe[0]);
            write(mypipe[1], resBuf, len);

            free(request);
            freeUrl(url);
            free(httpData);
            free(httpHead);

            exit(0);
        }
    }

    // 确保所有子进程结束后，父进程再结束
    while (wait(NULL) > 0);  // wait(NULL)是阻塞式的

    // 统计结果
    char resBuf[409600];
    memset(resBuf, 0, 409600);
    read(mypipe[0], resBuf, 409600);
    // printf("子进程数据：\n%s\n", resBuf);

    // 时长、总请求、每秒发送请求、成功、失败、每秒发送数据量、每秒接收数据量
    int maxTime, fotal, succeed, failed;
    unsigned long long writeBytes, readBytes;
    maxTime = fotal = succeed = failed = writeBytes = readBytes = 0;

    int j = 0;
    char *s1 = strchr(&resBuf[j], '\n');
    while (s1 != NULL) {
        int len = s1 - &resBuf[j];
        resBuf[j + len] = 0;

        int tmpTime, tmpSucceed, tmpFailed;
        unsigned long long tmpWriteBytes, tmpReadBytes;
        sscanf(&resBuf[j], "%d %d %d %llu %llu", 
            &tmpTime, &tmpSucceed, &tmpFailed, &tmpWriteBytes, &tmpReadBytes);
        if (tmpTime > maxTime) {
            maxTime = tmpTime;
        }
        succeed += tmpSucceed;
        failed += tmpFailed;
        writeBytes += tmpWriteBytes;
        readBytes += tmpReadBytes;

        j += (len + 1);
        s1 = strchr(&resBuf[j], '\n');
    }

    if (maxTime == 0) {
        maxTime = 1;
    }
    fotal = succeed + failed;
    printf("时长 %dseconds, 总请求 %d, 每秒发送请求 %d, 成功 %d, 失败 %d, "
        "每秒发送数据量 %llu, 每秒接收数据量 %llu\n",
        maxTime, fotal, fotal / maxTime, succeed, failed, 
        writeBytes / maxTime, readBytes / maxTime);
}

void clean() {
    free(httpHead);
    httpHead = NULL;
    free(httpData);
    httpData = NULL;
}
