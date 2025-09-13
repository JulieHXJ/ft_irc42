/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/08 15:49:16 by junjun            #+#    #+#             */
/*   Updated: 2025/09/13 21:16:20 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <csignal> // signal, SIGPIPE, SIG_IGN
#include <cstdlib> // std::atoi

int main(int argc, char** argv) {
	Server server;
    int port = (argc >= 2) ? std::atoi(argv[1]) : 6667; // default 6667
	
	// 防止向已关闭 socket 发送时触发 SIGPIPE 直接杀进程
    std::signal(SIGPIPE, SIG_IGN);
    server.serverInit(port);
	server.run();
	
	return 0;


	
	//the run() function logic:
	//accept a client connection
    sockaddr_in cli; 
	socklen_t len = sizeof(cli);
    int connfd = ::accept(listenfd, (sockaddr*)&cli, &len);//return a new fd if connected
    if (connfd < 0) { perror("accept"); return 1; }

    char ip[64]; std::memset(ip, 0, sizeof(ip));
    ::inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
    std::cout << "Client connected from " << ip << ":" << ntohs(cli.sin_port) << "\n";

	//loop to receive data and echo back
    char buf[4096];
    while (true) {
        ssize_t n = ::recv(connfd, buf, sizeof(buf), 0);
        if (n > 0) {
            // echo back
            ::send(connfd, buf, n, 0);
        } else if (n == 0) {
            std::cout << "Client closed connection.\n";
            break;
        } else {
            perror("recv");
            break;
        }
    }

    ::close(connfd);
    ::close(listenfd);
    return 0;
}

