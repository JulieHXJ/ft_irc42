/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/08 15:49:16 by junjun            #+#    #+#             */
/*   Updated: 2025/09/14 18:20:22 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include <csignal> // signal, SIGPIPE, SIG_IGN
#include <cstdlib> // std::atoi

/**
 * Main entry point for the IRC server.
 * Usage: ./ircserv [port]
 * Default port is 6667 if not specified.*/
int main(int argc, char** argv) {
	Server server;
    int port = (argc >= 2) ? std::atoi(argv[1]) : 6667; // default 6667
	
    //welcome message
    std::cout << "Starting server on port " << port << "...\n";
	// avoiding SIGPIPE signal killing the process
    std::signal(SIGPIPE, SIG_IGN);
    server.serverInit(port);
	server.run();//incomplete yet
	return 0;
}



/**
 * Simple echo server.
 * Usage: ./ircserv
 * in another terminal nc 127.0.0.1 6667
 */
// #include <iostream>
// #include <cstring>
// #include <cstdlib>
// #include <cstdio>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>

// int main() {
//     int listenfd = ::socket(AF_INET, SOCK_STREAM, 0);
//     if (listenfd < 0) { perror("socket"); return 1; }

//     int yes = 1;
//     ::setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

//     sockaddr_in addr; std::memset(&addr, 0, sizeof(addr));
//     addr.sin_family = AF_INET;
//     addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 0.0.0.0
//     addr.sin_port = htons(6667);

//     if (::bind(listenfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
//         perror("bind"); return 1;
//     }
//     if (::listen(listenfd, 16) < 0) { perror("listen"); return 1; }

//     std::cout << "Listening on 0.0.0.0:6667 ..." << std::endl;

//     sockaddr_in cli; socklen_t len = sizeof(cli);
//     int connfd = ::accept(listenfd, (sockaddr*)&cli, &len);
//     if (connfd < 0) { perror("accept"); return 1; }

//     char buf[4096];
//     while (true) {
//         ssize_t n = ::recv(connfd, buf, sizeof(buf), 0);
//         if (n > 0) {
//             ::send(connfd, buf, n, 0); // 原样回显
//         } else if (n == 0) {
//             std::cout << "client closed" << std::endl;
//             break;
//         } else {
//             perror("recv");
//             break;
//         }
//     }
//     ::close(connfd);
//     ::close(listenfd);
//     return 0;
// }