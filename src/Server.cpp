/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 20:06:12 by junjun            #+#    #+#             */
/*   Updated: 2025/09/28 15:46:20 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include <fcntl.h>
#include <unistd.h> // close
#include <arpa/inet.h> // inet_ntop
#include <cerrno>
#include <cstring>
#include <stdexcept>

#include <csignal>
extern volatile sig_atomic_t g_stop;// define in main (“There is a variable called g_stop, it’s declared elsewhere, it can change suddenly (signal), and I want to use it here.”)

namespace {
	
	const size_t MAX_OUTBUF = 256 * 1024; // 256 KB per client
	void logErr(const char* str){ perror(str); }
	
	void logNew(int fd, const sockaddr_in& clientAddr){ 
		char ipStr[INET_ADDRSTRLEN];
		::inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
		std::cout << "[+] New connection from " << ipStr << ":" 
					<< ntohs(clientAddr.sin_port) << ", fd=" << fd << "\n";}
	
	void logClose(int fd){ std::cout << "[-] fd=" << fd << " closed\n"; }
}


//helpers
void Server::setNonBlocking(int fd){
	int flags = ::fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
        throw std::runtime_error(std::string("fcntl(F_GETFL): ") + std::strerror(errno));
    }
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error(std::string("fcntl(F_SETFL,O_NONBLOCK): ") + std::strerror(errno));
    }
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

//server functions
void Server::closeFds(){
	for (size_t i = 0; i < pollfds.size(); ++i) {
		close(pollfds[i].fd);
	}
	pollfds.clear();
	inbuff.clear();
	outbuff.clear();
}


void Server::serverInit(int port){ 
	//1) Create socket. ipv4, tcp
	listenfd = ::socket(AF_INET, SOCK_STREAM, 0); 
	if (listenfd < 0) { 
		throw std::runtime_error(std::string("socket: ") + std::strerror(errno));
	} 
	
	//2) allow socket descriptor to be reusable
	//setsockopt() is used to set options for the socket referred to by the file descriptor sockfd.
	//Here, it sets the SO_REUSEADDR option, which allows the socket to bind to an address that is already in use.
	//This is useful for server applications that need to restart and bind to the same port without waiting for the OS to release it.
	int yes = 1;
    if (::setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        ::close(listenfd);
        throw std::runtime_error(std::string("setsockopt(SO_REUSEADDR): ") + std::strerror(errno));
    }
#ifdef SO_REUSEPORT
    if (::setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) < 0) {
        ::close(listenfd);
        throw std::runtime_error(std::string("setsockopt(SO_REUSEPORT): ") + std::strerror(errno));
    }
#endif
	
	//3) bind
	sockaddr_in addr; 
	std::memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // bind to all local IPv4 interfaces 
	addr.sin_port = htons(port); //bind the socket to IP and port 
	if (::bind(listenfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        ::close(listenfd);
        throw std::runtime_error(std::string("bind: ") + std::strerror(errno));
    }
	
	// 4) listen
    if (::listen(listenfd, 64) < 0) {
        ::close(listenfd);
        throw std::runtime_error(std::string("listen: ") + std::strerror(errno));
    }
	// 5) non-block + poll vector
	setNonBlocking(listenfd);
	pollfds.clear();
	addPollFd(pollfds, listenfd, POLLIN);

	std::cout << "Listening on 0.0.0.0:" << port << " ... (waiting clients)\n";
}

//main loop

/**
 * @brief Accept new incoming connections on the listening socket. 
 * Print welcone message and the client info.
 */
void Server::acceptNew(){
    if (pollfds.empty() || !(pollfds[0].revents & POLLIN)) return;//no new connection

    for(;;){
        sockaddr_in clientAddr; socklen_t clientLen = sizeof(clientAddr);
        int connfd = ::accept(listenfd, (sockaddr*)&clientAddr, &clientLen);
        if (connfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            if (errno == EINTR) continue;
            logErr("accept"); break;
        }

        setNonBlocking(connfd);
        addPollFd(pollfds, connfd, POLLIN);


		//build a client object here and store it
		// Client c;
		// c.fd = connfd;

        // initialize buffer for this client
        inbuff[connfd].clear();
        outbuff[connfd] += ":Server NOTICE * :🎉🎉 Yo! Welcome to *Club42 Chatroom* 🍹\r\n";
        // enable write for the newly added (it's at the back)
        pollfds.back().events |= POLLOUT;

        logNew(connfd, clientAddr);
    }
}

/**
 * @brief Close the socket, erase buffers, and remove from pollfds.
 */
void Server::cleanupIndex(size_t i){
    int fd = pollfds[i].fd;
    logClose(fd);
    ::close(fd);
    inbuff.erase(fd);
    outbuff.erase(fd);
    // TODO: remove from channels, nick maps when you add them
    removePollFd(pollfds, i); //pop back swap
}

/**
 * @brief keep receiving and appending until EAGAIN,
 * then extract lines, process command, generate responses
 */
bool Server::handleReadable(size_t i){
	if (i >= pollfds.size()) return false; //safety check
	int fd = pollfds[i].fd;
    char buf[4096];
	// 1) read in buffer
	for(;;){
        ssize_t r = ::recv(fd, buf, sizeof(buf), 0);
        if (r > 0) {
            inbuff[fd].append(buf, static_cast<size_t>(r));
        } else if (r == 0) {
			//client closed connection
            cleanupIndex(i);
            return true; // inde was swap-removed; caller must not ++i
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            logErr("recv");
            cleanupIndex(i);
            return true;
        }
    }
	
	// 2) parse complete lines & act
	std::string line;
	while (getLine(inbuff[fd], line)) {
		// trim spaces
		while (!line.empty() && (line[0]==' ' || line[0]=='\t')) line.erase(0,1);
		// handleCmd(fd, line); // todo
        if (line == "QUIT") {
            ::shutdown(fd, SHUT_RDWR); // next poll/recv sees close
            continue;
        }
        if (line == "WHO") {
            std::string reply = "USERS:";
            for (size_t k = 1; k < pollfds.size(); ++k)
                reply += " fd=" + std::to_string(pollfds[k].fd);
            outbuff[fd] += reply + "\r\n";
            // ensure POLLOUT on this fd
            pollfds[i].events |= POLLOUT;
            continue;
        }

        // broadcast to all other clients
        for (size_t j = 1; j < pollfds.size(); ++j) {
            int other = pollfds[j].fd;
            if (other == fd) continue;
            if (outbuff[other].size() + line.size() + 2 <= MAX_OUTBUF) {
                outbuff[other] += line;
                outbuff[other] += "\r\n";
                pollfds[j].events |= POLLOUT;
            }
        }
	}
	
    // If output is nnot empty (from WHO/welcome), keep POLLOUT on
    if (!outbuff[fd].empty()) {
        pollfds[i].events |= POLLOUT;
	}
	return false; // not removed
}

bool Server::handleWritable(size_t i){
	if (i >= pollfds.size()) return false; //safety check
	int fd = pollfds[i].fd;

	std::string &ob = outbuff[fd];
	while (!ob.empty()){
		ssize_t w = ::send(fd, ob.data(), ob.size(), 0);
		if (w > 0) {
			ob.erase(0, static_cast<size_t>(w));// remove sent data
		} else if (w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			return false; // cannot send more now (对方 TCP 接收窗口满了、内核发送缓冲不够等)
		} else {
			logErr("send");
			cleanupIndex(i);
			return true;//index is removed
		}
	}
	pollfds[i].events = POLLIN;//all sent
	return false;
}

void Server::run(){
	while (true)
	{
		if (g_stop) break; // exit loop
		int n = ::poll(&pollfds[0], pollfds.size(), 500);// 500ms timeout to check g_stop
		if (n < 0) {
			if (errno == EINTR) continue; // interrupted by signal, retry
			throw std::runtime_error(std::string("poll: ") + std::strerror(errno));
		}
	
		//1. accept new connections
		acceptNew();
		

		//2. handle each client socket， increment i only if not removed
		for (size_t i = 1; i < pollfds.size();)
		{
			int fd = pollfds[i].fd;
			short re = pollfds[i].revents;
			bool removed = false;

			//2.1 Errors/Hangups (POLLERR | POLLHUP | POLLNVAL)
			if (re & (POLLERR | POLLHUP | POLLNVAL))
			{
				cleanupIndex(i);
				removed = true;
			}
			
			//2.2 handle readable fds
			if (!removed && (re & POLLIN))
			{
				removed = handleReadable(i);
				// if i was removed in handleReadable
				if (i >= pollfds.size() || pollfds[i].fd != fd) {
					removed = true;
				}
			if (!removed && (re & POLLOUT)) {
				removed = handleWritable(i);
			}

            if (!removed) ++i; // manually increment if not removed
        	}
		}
    }
}
