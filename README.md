# 简介

一个用于压测的项目，支持http协议。


程序开启多进程来进行压测，每个进程代表一个客户端


参考资料：https://github.com/EZLippi/WebBench

# 使用方法

案例1：调试，向目标接口发送一个请求

```text
[root@Linux1 web_bench]# ./wb http://www.taobao.com
请求：
GET / HTTP/1.1
Host: www.taobao.com
User-Agent: WebBench 0.0.1
Connection: close


如果请求只有一次，会打印响应信息，帮助用户调试：
HTTP/1.1 301 Moved Permanently
Server: Tengine
Date: Sat, 29 Jun 2024 04:41:25 GMT
Content-Type: text/html
Content-Length: 262
Connection: close
Location: https://www.taobao.com/
Via: cache11.cn414[,0]
Timing-Allow-Origin: *
EagleId: 7d27879f17196360856825089e

<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">
<html>
<head><title>301 Moved Permanently</title></head>
<body>
<h1>301 Moved Permanently</h1>
<p>The requested resource has been assigned a new permanent URI.</p>
<hr/>Powered by Tengine</body>
</html>


时长 1seconds, 总请求 1, 每秒发送请求 1, 成功 0, 失败 1, 每秒发送数据量 82, 每秒接收数据量 535
```


案例2：压测，开启10个客户端，每个客户端请求1000次，同时指定请求体

```text
[root@Linux1 web_bench]# ./wb -c 10 --count 1000  -X POST --data '{"aa": "bb"}' http://192.168.31.147:8080/api/v1/hello/test2
请求：
POST /api/v1/hello/test2 HTTP/1.1
Host: 192.168.31.147
User-Agent: WebBench 0.0.1
Connection: close
Content-Type: text/plain
Content-Length: 12

{"aa": "bb"}

时长 19seconds, 总请求 10000, 每秒发送请求 526, 成功 10000, 失败 0, 每秒发送数据量 83157, 每秒接收数据量 75152
```