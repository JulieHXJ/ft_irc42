/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 20:06:12 by junjun            #+#    #+#             */
/*   Updated: 2025/09/14 18:15:33 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include <fcntl.h>
#include <unistd.h> // close
#include <arpa/inet.h> // inet_ntop

void Server::closeFds(){
	for (size_t i = 0; i < pollfds.size(); ++i) {
		close(pollfds[i].fd);
	}
	pollfds.clear();
	clientBuf.clear();
}

void Server::setNonBlocking(int fd){
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		// perror("fcntl F_GETFL");
		// return;
		flags = 0;//allow it to proceed, even if error
	}
	::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

//add a fd to pollfd list
void Server::addPollFd(std::vector<pollfd>& pfds, int sckfd, short events) {
	struct pollfd pfd;
	pfd.fd = sckfd;
	pfd.events = events;
	pfd.revents = 0;
	pfds.push_back(pfd);// add to the end
}

//remove a selected fd from pollfd list by index
void Server::removePollFd(std::vector<pollfd>& pfds, size_t index) {
    pfds[index] = pfds.back();// swap with the last one
    pfds.pop_back();
}

void Server::serverInit(int port){ 
	//Create the listening socket. 
	listenfd = ::socket(AF_INET, SOCK_STREAM, 0); 
	if (listenfd < 0) { 
		perror("socket"); 
		std::exit(1);
	} 
	
	//allow socket descriptor to be reusable
	//setsockopt() is used to set options for the socket referred to by the file descriptor sockfd.
	//Here, it sets the SO_REUSEADDR option, which allows the socket to bind to an address that is already in use.
	//This is useful for server applications that need to restart and bind to the same port without waiting for the OS to release it.
	int yes = 1; 
	::setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)); 
#ifdef SO_REUSEPORT 
	::setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)); 
#endif 
	
	//fill addr structure 
	sockaddr_in addr; 
	std::memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // bind to all local IPv4 interfaces 
	addr.sin_port = htons(port); //bind the socket to IP and port 
	if (::bind(listenfd, (sockaddr*)&addr, sizeof(addr)) < 0) { 
		perror("bind"); 
		std::exit(1);
	} 
	
	//change socket into listening mode, max 8 pending connections 
	if (::listen(listenfd, 8) < 0) { 
		perror("listen"); 
		std::exit(1);
	} 
	
	//Build the pollfd list with the listening socket (and maybe the self-pipe read end). 
	setNonBlocking(listenfd); 
	pollfds.clear();
    addPollFd(pollfds, listenfd, POLLIN);

	std::cout << "Listening on 0.0.0.0:" << port << " ... (waiting 1 client)\n";
}

//extract a line from buf, return true if a line is found
bool Server::getLine(std::string& clientBuf, std::string& line) {
    std::string::size_type pos = clientBuf.find('\n');
    if (pos == std::string::npos) return false;
    line = clientBuf.substr(0, pos);
    // remove '\r' if present
    if (!line.empty() && line[line.size()-1] == '\r') line.erase(line.size()-1);
    clientBuf.erase(0, pos + 1); // remove this line + '\n'
    return true;
}

void Server::run(){
	//placeholder to keep the server running
	std::cout << "[run] placeholder: serverInit done. (run() not implemented yet)\n";
    std::cout << "Press Ctrl+C to exit.\n";
    for (;;) { ::pause(); }

	// //real server loop with non-blocking + poll (multi-client echo, line-based)
	// while (true)
	// {
	// 	//poll()：等待任意 fd 有事件（读/写/错误）
	// 	int n = ::poll(&pollfds[0], pollfds.size(), -1);
	// 	if (n < 0) {
	// 		if (errno == EINTR)
	// 		{
	// 			continue; // interrupted by signal, retry
	// 		}
	// 		perror("poll");
	// 		break;
	// 	}
	
	// 	//1. accept new connections 监听 fd 可读 → accept() 直到 EAGAIN

	// 	//2. 遍历客户端 fd：
	// 	for (size_t i = 0; i < count; i++)
	// 	{

	// 		//2.1 handle readable fds
	// 		// POLLIN → recv() 循环读入 inbuf（直到 EAGAIN）→ 按 \n 切出完整行（去尾部 \r）→ 解析命令 → 产出响应（写入 outbuf）→ 若 outbuf 非空，打开 POLLOUT
	// 		//extract line from clientBuf, process it, append response to sentbuf


			
	// 		//2.2 handle writable from sendbuf
	// 		// POLLOUT → 从 outbuf 尽量 send()（直到 EAGAIN）→ 发送完则关闭 POLLOUT

			
			
	// 		//2.3 POLLHUP/POLLERR 或 recv()==0 → 清理客户端（离开所有频道、关闭 fd、移除数据结构）
		


	// 	}
	
	// }
	
}
