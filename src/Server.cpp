/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 20:06:12 by junjun            #+#    #+#             */
/*   Updated: 2025/09/27 00:52:18 by junjun           ###   ########.fr       */
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
	inbuff.clear();
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

/**
 * @brief Add a fd to pollfd list
 */
void Server::addPollFd(std::vector<pollfd>& pfds, int sckfd, short events) {
	struct pollfd pfd;
	pfd.fd = sckfd;
	pfd.events = events;
	pfd.revents = 0;
	pfds.push_back(pfd);// add to the end
}

/**
 * @brief Remove a fd from pollfd list by index
 * @note swap with the last one and pop_back
 */
void Server::removePollFd(std::vector<pollfd>& pfds, size_t index) {
    pfds[index] = pfds.back();
    pfds.pop_back();
}

/**
 * @brief Extract a line ending with '\n' from the input buffer.
 * 
 * @param inbuff The input buffer containing received data.
 * @param line The extracted line without the trailing '\n' or '\r'.
 * @note 1. find the first '\n'
 * 2. extract the line without '\n'
 * 3. remove '\r' if present
 * 4. remove this line (with '\n') from inbuff 
 */
bool Server::getLine(std::string& inbuff, std::string& line) {
	std::string::size_type pos = inbuff.find('\n'); 
	if (pos == std::string::npos) return false; 
	line = inbuff.substr(0, pos);
	if (!line.empty() && line[line.size()-1] == '\r') line.erase(line.size()-1);
	inbuff.erase(0, pos + 1); 
	return true;
}

void Server::serverInit(int port){ 
	//Create the listening socket. ipv4, tcp
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

void Server::run(){
	// placeholder to keep the server running
	// std::cout << "[run] placeholder: serverInit done. (run() not implemented yet)\n";
    // std::cout << "Press Ctrl+C to exit.\n";
    // for (;;) { ::pause(); }

	//real server loop with non-blocking + poll (multi-client, line-based)
	while (true)
	{
		int n = ::poll(&pollfds[0], pollfds.size(), -1);
		if (n < 0) {
			if (errno == EINTR)
				continue; // interrupted by signal, retry
			perror("poll");
			break;
		}
	
		//1. found new connection on listening socket, call accept()
		if (!pollfds.empty() && (pollfds[0].revents & POLLIN)) {
			while (true)
			{
				sockaddr_in clientAddr;
				socklen_t clientLen = sizeof(clientAddr);
				int connfd = ::accept(listenfd, (sockaddr*)&clientAddr, &clientLen);
				if (connfd < 0) {
					if (errno == EAGAIN || errno == EWOULDBLOCK)
						break; // no more incoming connections
					perror("accept");
					continue;
				}
				setNonBlocking(connfd);
				addPollFd(pollfds, connfd, POLLIN);
				
				inbuff[connfd] = ""; // initialize buffer for this client
				// outbuff[connfd] = ""; // initialize send buffer for this client
				outbuff[connfd] += "server NOTICE * :Welcome!\r\n";
				pollfds.back().events |= POLLOUT; // we just added connfd at the back


				char ipStr[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
				std::cout << "New connection from " << ipStr << ":" << ntohs(clientAddr.sin_port) 
							<< ", fd=" << connfd << "\n";
			}
		}
		

		//2. handle each client socket， increment i only if not removed
		for (size_t i = 1; i < pollfds.size();)
		{
			int fd = pollfds[i].fd;
			short re = pollfds[i].revents;
			bool removed = false;

			//2.1 Errors/Hangups (POLLERR | POLLHUP | POLLNVAL):close fd, erase from maps/poll list.
			//清理客户端（离开所有频道、关闭 fd、移除数据结构）
			if (re & (POLLERR | POLLHUP | POLLNVAL))
			{
				std::cout << "[-] client fd=" << fd << " closed (err/hup)\n";
				::close(fd);
				inbuff.erase(fd);
				outbuff.erase(fd);
				//todo: remove from channels, nicknames, etc.
				removePollFd(pollfds, i); //pop back swap
				removed = true;
			}
			
			//2.2 handle readable fds: keep receiving and appending until EAGAIN, then extract lines, process them, generate responses
			// POLLIN → recv() 循环读入 inbuf（直到 EAGAIN）→ 按 \n 切出完整行（去尾部 \r）→ 解析命令 → 产出响应（写入 outbuf）→ 若 outbuf 非空，打开 POLLOUT
			if (!removed && (re & POLLIN))
			{
				char buf[4096];
				while (true)
				{
					//
					ssize_t r = ::recv(fd, buf, sizeof(buf), 0);
					if (r > 0){
						inbuff[fd].append(buf, r);
					} else if (r == 0) {
						//client closed connection
						std::cout << "[-] client fd=" << fd << " closed\n";
						::close(fd);
						inbuff.erase(fd);
						outbuff.erase(fd);
						removePollFd(pollfds, i); //pop back swap
						removed = true;
						break;
					} else {
						if (errno == EAGAIN || errno == EWOULDBLOCK)
							break; // no more data
						perror("recv");
						::close(fd);
						inbuff.erase(fd);
						outbuff.erase(fd);
						removePollFd(pollfds, i); //pop back swap
						removed = true;
						break;
					}
				}

				// parse complete lines from inbuff and echo back (line + CRLF)
				if (!removed)
				{
					std::string line;
					while (getLine(inbuff[fd], line)) {
						//echo back only
						// outbuff[fd] += line;
						// outbuff[fd] += "\r\n";


						
						//send client message to other clients
						for (size_t j = 1; j < pollfds.size(); ++j) {
							int other = pollfds[j].fd;
							if (other == fd) continue;// skip sender 
							outbuff[other] += line;
							outbuff[other] += "\r\n";
							pollfds[j].events |= POLLOUT;
						}

						//todo: parse IRC commands here, generate responses into outbuff (also need to handle empty input)
					}
					
					// if outbuff is not empty, enable POLLOUT
					if (!outbuff[fd].empty()) {
						pollfds[i].events = POLLIN | POLLOUT; // enable write readiness
					}
				}
			}
			
			//2.3 handle writable from outbuff
			// POLLOUT → 从 outbuf 尽量 send()（直到 EAGAIN）→ 发送完则关闭 POLLOUT
			if (!removed && (re & POLLOUT))
			{
				std::string &ob = outbuff[fd];
				while (!ob.empty())
				{
					ssize_t w = ::send(fd, ob.data(), ob.size(), 0);
					if (w > 0) {
						ob.erase(0, w);// remove sent data
					} else if (w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
						break; // cannot send more now (对方 TCP 接收窗口满了、内核发送缓冲不够等)
					} else {
						perror("send");
						::close(fd);
						inbuff.erase(fd);
						outbuff.erase(fd);
						removePollFd(pollfds, i);
						removed = true;
						break;
					}
				}
				
				if (!removed && ob.empty()) {
					pollfds[i].events = POLLIN;//all sent
				}
			}

            if (!removed) ++i; // manually increment if not removed
        }
    }
}
