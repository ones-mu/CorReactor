# getaddrinfo函数的作用

使用getaddrinfo函数用于socket获取用于监听/通信的文件描述符前

getaddrinfo() 可以一次性解决：

    1. ​​主机名解析​​（域名 → IP）
    ​2. ​端口号/服务名解析​​（如 "http" → 80）
    ​3. ​自动选择 IPv4/IPv6​​（通过 hints.ai_family = AF_UNSPEC）
    ​4. ​返回可直接使用的 struct sockaddr​​（无需手动填充 sockaddr_in 或 sockaddr_in6）

## 对比感受

​传统方式（IPv4 硬编码）​​ （客户端）

```c++
struct sockaddr_in addr;
addr.sin_family = AF_INET;// IPv4
addr.sin_port = htons(80);
inet_pton(AF_INET, "192.168.1.1", &addr.sin_addr);

int lfd = socket(AF_INET, SOCK_STREAM, 0);
connect(lfd, (struct sockaddr*)&addr, sizeof(addr));
```

一个字麻烦，要选用ipv4还是ipv6，要手动填写端口号，要手动填写ip地址。

如果使用getaddrinfo函数：

```c++
struct addrinfo hints, *res;
memset(&hints, 0, sizeof hints);
hints.ai_family = AF_UNSPEC;  // 支持 IPv4 和 IPv6
hints.ai_socktype = SOCK_STREAM;  // TCP

getaddrinfo("www.example.com", "80", &hints, &res);

// res 已包含所有必要信息（IP、端口、协议族等）
int lfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
connect(lfd, res->ai_addr, res->ai_addrlen);

freeaddrinfo(res);  // 释放内存
```

### 服务器端用法

```c++
struct addrinfo hints, *res;
memset(&hints, 0, sizeof hints);
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;  // 用于bind (服务器端需要这个，否则可能只能本地访问)

getaddrinfo(NULL, "8080", &hints, &res);  // NULL表示本地地址
socket(res->ai_family, res->ai_socktype, res->ai_protocol);
bind(sockfd, res->ai_addr, res->ai_addrlen);
```

## 链表结构

struct addrinfo 是一个链表结构，用于存储通过 getaddrinfo() 函数获取的地址信息。这个设计允许返回多个可能的地址结果，因为一个主机名可能对应多个 IP 地址（比如 IPv4 和 IPv6），或者一个服务可能对应多个端口/协议组合。
