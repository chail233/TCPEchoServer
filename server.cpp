#include <iostream>
#include <cstring>
#include <cstdio>
#include <sys/socket.h> //socket相关函数（linux）
#include <netinet/in.h> //sockaddr_in结构体
#include <unistd.h>
#include <arpa/inet.h> //inet_ntoa函数
int main() {
    //1.创建socket
    //AF_INET  使用ipv4协议
    //SOCK_STREAM TCP协议
    //自动选择协议  对于SOCK_STREAM是TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }
    std::cout << "Socket created. File Descriptor:" << server_fd << std::endl;

    //2.准备地址结构体
    sockaddr_in address{};//sockaddr_in是专门给Ipv4用的地址结构体
    std::memset(&address, 0, sizeof(address)); //先用0填充结构体，保证干净
    address.sin_family = AF_INET;//地址族:IPv4
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    //3.绑定
    int bind_result = bind(server_fd, (sockaddr*)&address, sizeof(address));
    if (bind_result == -1) {
        std::cerr << "Error binding socket" << std::endl;
        close(server_fd);
        return 1;
    }
    std::cout << "Socket Binded. Port:8080" << std::endl;

    //4.开始监听
    //SOMAXCONN:系统允许的最大等待队列长度
    if (listen(server_fd, SOMAXCONN) == -1) {
        std::cerr << "Error listening socket" << std::endl;
        close(server_fd);
        return 1;
    }
    std::cout << "Listening...Waiting for client connection" << std::endl;

    //5.接受客户端连接
    sockaddr_in client_addr{}; // 用来存储客户端地址信息
    socklen_t client_len = sizeof(client_addr);// 地址结构体大小
    //accept会阻塞，直到有客户端连接进来
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        std::cerr << "Error accepting client" << std::endl;
        close(server_fd);
        return 1;
    }
    char client_ip[INET_ADDRSTRLEN];//用来存放ip字符串
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    int client_port = ntohs(client_addr.sin_port);
    std::cout << "Client connected. IP:" << client_ip << ":" << client_port << std::endl;

    //6.收发数据
    char buffer[1024];
    std::string leftover;//存放未传输完的数据

    while (true) {
        //recv()也是阻塞的，等待客户端发数据过来
        int bytes_read = recv(client_fd, buffer, sizeof(buffer)-1, 0);
        if (bytes_read == -1) {
            std::cerr << "Error reading from client" << std::endl;
            break;
        }
        if (bytes_read == 0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        }
        buffer[bytes_read] = '\0';
        leftover += std::string(buffer, bytes_read);//拼接
        size_t pos;
        //处理每一条数据
        while ((pos = leftover.find('\n')) != std::string::npos) {
            std::string line = leftover.substr(0, pos);
            leftover = leftover.substr(pos + 1);
            std::cout << "Received Line:" << line << std::endl;
            std::string reply = line + "\n";
            send(client_fd, reply.c_str(), reply.size(), 0);
            std::cout << "Sent Line:" << reply << std::endl;
        }
    }

    close(server_fd);
    close(client_fd);
    std::cout << "Server Shut Down." << std::endl;
    return 0;
}