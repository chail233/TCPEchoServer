#include <iostream>
#include <cstring>
#include <cstdio>
#include <sys/socket.h> //socket相关函数（linux）
#include <netinet/in.h> //sockaddr_in结构体
#include <unistd.h>
#include <arpa/inet.h> //inet_ntoa函数
#include <signal.h> //signal函数
#include <sys/wait.h> //waitpid 函数

//处理子进程结束的信号,防止僵尸进程
void handle_signal(int signo) {
    //waitpid(-1, ...)表示回收任意一个已结束的子进程
    //WNOHANG 表示如果没有已结束的子进程，不要阻塞直接返回
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
int main() {
    //注册信号处理函数
    //SIGCHLD 子进程结束时， 内核会发给父进程这个信号
    signal(SIGCHLD, handle_signal);

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
    //设置socket选项：允许重用地址（方便重启服务器）
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
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
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd==-1) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::cout << "New Connected Client IP: " << client_ip << std::endl;
        //fork()创建子进程进行通信
        pid_t pid = fork();
        if (pid == -1) {
            std::cerr << "Error forking process" << std::endl;
            close(client_fd);
            continue;
        }
        if (pid == 0) {
            //关闭子进程中的监听Socket
            close(server_fd);
            std::cout << "[Child Process " << getpid() << "] Start To Serve Client" << client_ip << std::endl;
            char buffer[1024];
            std::string leftover;
            while (true) {
                int bytes_read = recv(client_fd, buffer, sizeof(buffer)-1, 0);
                if (bytes_read==0) {
                    std::cout << "[Child Process " << getpid() << "] Client disconnected" << std::endl;
                    break;
                }
                if (bytes_read == -1) {
                    std::cerr << "Error reading from client socket" << std::endl;
                    break;
                }
                buffer[bytes_read] = '\0';
                leftover += std::string(buffer, bytes_read);
                size_t pos;
                while ((pos = leftover.find('\n')) != std::string::npos) {
                    std::string line = leftover.substr(0, pos);
                    leftover = leftover.substr(pos + 1);
                    std::cout << "[Child Process " << getpid() << "] Received line: " << line << std::endl;
                    std::string reply = line+"\n";
                    send(client_fd, reply.c_str(), reply.size(), 0);
                }
            }
            close(client_fd);
            std::cout << "[Child Process " << getpid() << "] End To Serve Client" << std::endl;
            return 0;//子进程退出
        }
        close(client_fd);
        std::cout << "[Parent Process] Wait For New Connection..." << std::endl;
    }
    close(server_fd);
    std::cout << "Server Shut Down." << std::endl;
    return 0;
}