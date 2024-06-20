# 简介

一个用于压测的项目，支持http协议。


案例1：

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


案例2：

```text
[root@Linux1 web_bench]# ./wb -c 10 --count 1000  -X POST --data '{"aa": "bb", \}' http://192.168.31.147:8080/api/v1/hello/test2
请求：
POST /api/v1/hello/test2 HTTP/1.1
Host: 192.168.31.147
User-Agent: WebBench 0.0.1
Connection: close
Content-Type: text/plain
Content-Length: 15

{"aa": "bb", \}

时长 2seconds, 总请求 10000, 每秒发送请求 5000, 成功 10000, 失败 0, 每秒发送数据量 805000, 每秒接收数据量 713947
```