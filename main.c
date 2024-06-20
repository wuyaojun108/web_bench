#include "web_bench.h"

// 教程地址：https://github.com/EZLippi/WebBench

int main(int argc, char *const *argv) {
    // 解析参数
    if (parseParam(argc, argv) == -1) {
        return 0;
    }

    // 解析url
    Url u = URL_INIT;
    parseUrl(url, &u);

    // 构建请求
    char *request = buildRequest(&u);

    // 开始压测
    bench(request, &u);

    clean();
    freeUrl(&u);
    free(request);
    request = NULL;
    return 0;
}
