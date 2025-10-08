/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 20:06:12 by junjun            #+#    #+#             */
/*   Updated: 2025/10/08 18:56:13 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"

extern volatile sig_atomic_t g_stop;// define in main (“There is a variable called g_stop, it’s declared elsewhere, it can change suddenly (signal), and I want to use it here.”)

static const size_t MAX_OUTBUF = 256 * 1024;

namespace {

	void logNew(int fd, const sockaddr_in& a){
        char ip[INET_ADDRSTRLEN] = {0};
        ::inet_ntop(AF_INET, &a.sin_addr, ip, sizeof(ip));
        std::cout << "[+] New connection from " << ip << ":" << ntohs(a.sin_port)
                  << ", fd=" << fd << "\n";
    }
	
	// void logClose(int fd){ std::cout << "[-] fd=" << fd << " closed\n"; }
}

Server::~Server(){
    for (size_t i = 0; i < pollfds.size(); ++i) {
        ::close(pollfds[i].fd);
    }
    std::map<int, Client*>::iterator it = fd2client.begin();//do i need it?
    for (; it != fd2client.end(); ++it) delete it->second;
}

//helpers
void Server::setNonBlocking(int fd){
	int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1) flags = 0;
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error(std::string("fcntl(O_NONBLOCK): ")
                                 + std::strerror(errno));
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
	if (pfds.empty() | index >= pfds.size())
        return; // safety guard

    pfds[index] = pfds[ pfds.size() - 1 ];  // copy last element into index
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

//todo: client functions
void Client::sendMessage(const std::string& line) {
    // ensure CRLF; server will not add if already present
    std::string out = line;
    if (out.size() < 2 || out[out.size()-2] != '\r' || out[out.size()-1] != '\n')
        out += "\r\n";
    if (server_) server_->pushLine(fd_, out);
}


/* ============================ server functions ============================ */
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
		throw std::runtime_error("socket failed");
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
        throw std::runtime_error("bind failed");
    }
	
	// 4) listen
    if (::listen(listenfd, 64) < 0) {
		::close(listenfd);
        throw std::runtime_error("listen failed");
    }
	
	setNonBlocking(listenfd);
	pollfds.clear();
	addPollFd(pollfds, listenfd, POLLIN);
	
	std::cout << "Listening on 0.0.0.0:" << port << " ... (waiting clients)\n";
}

//main loop
void Server::run(){
	while (!g_stop)
	{
		if (pollfds.empty()) break; // exit loop if no fds to monitor
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
			if (re & (POLLERR | POLLHUP | POLLNVAL)) {
				cleanupIndex(i);
				removed = true;
			}
			
			//2.2 handle readable fds
			if (!removed && (re & POLLIN)) {
				removed = handleReadable(i);
				if (removed) continue;
			}
			//2.3 handle writable fds
			if (!removed && (re & POLLOUT)) {
				removed = handleWritable(i);
				if (removed) continue;
			}
			if (!removed) ++i; // manually increment if not removed
		}
	}
}



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

        // initialize buffer for this client
        inbuff[connfd].clear();
		outbuff[connfd].clear();
		
        // outbuff[connfd] += ":Server NOTICE * :🎉🎉 Yo! Welcome to *Club42 Chatroom* 🍹\r\n";
		// create Client
        Client* c = new Client(cfd);
        c->attachServer(this);
        fd2client[cfd] = c;

        // greeting
        c->sendMessage(":"
            + std::string(SERVER_NAME)
            + " NOTICE * :Welcome to Club42 ✨");




		
        // enable write for the newly added (it's at the back)
        pollfds[ pollfds.size() - 1 ].events |= POLLOUT;
		
        logNew(connfd, clientAddr);
    }
}

/**
 * @brief Close the socket, erase buffers, and remove from pollfds.
 */
void Server::cleanupIndex(size_t i){
    int fd = pollfds[i].fd;

	 // remove from channel membership
	 removeClientFromAllChannels(fd);

	 // erase nick map entry if any
	
    
    ::close(fd);
    inbuff.erase(fd);
    outbuff.erase(fd);
    removePollFd(pollfds, i); //pop back swap
	std::cout << "[-] fd=" << fd << " closed\n";
}

/**
 * @brief keep receiving and appending until EAGAIN,
 * then extract lines, process command, generate responses
 */
bool Server::handleReadable(size_t i){
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
            std::perror("recv");
            cleanupIndex(i);
            return true;
        }
    }
	
	// 2) parse complete lines & act
	std::string line;
	while (getLine(inbuff[fd], line)) {
		// trim spaces
		while (!line.empty() && (line[0]==' ' || line[0]=='\t')) line.erase(0,1);
		if (line.empty()) continue;
		
        // if (line == "KILL") {
		// 	cleanupIndex(i);
        //     return true;
        // }
        // if (line == "WHO") {
		// 	std::ostringstream oss;
		// 	oss << "USERS:";
        //     for (size_t k = 1; k < pollfds.size(); ++k) {
		// 		oss << "fd = " << pollfds[k].fd;
		// 	}
        //     outbuff[fd] += oss.str() + "\r\n";
        //     // ensure POLLOUT on this fd
        //     pollfds[i].events |= POLLOUT;
        //     continue;
        // }
		
		handleCmd(fd, line); // todo: command handler functions in Server_cmd.cpp
		
        // broadcast to all other clients
        // for (size_t j = 1; j < pollfds.size(); ++j) {
		// 	int other = pollfds[j].fd;
        //     if (other == fd) continue;
        //     if (outbuff[other].size() + line.size() + 2 <= MAX_OUTBUF) {
		// 		outbuff[other] += line;
        //         outbuff[other] += "\r\n";
        //         pollfds[j].events |= POLLOUT;
        //     }
        // }
	}
	
    // If output is nnot empty (from WHO/welcome), keep POLLOUT on
    if (!outbuff[fd].empty()) {
        pollfds[i].events |= POLLOUT;
	}
	return false; // not removed
}

bool Server::handleWritable(size_t i){
	int fd = pollfds[i].fd;
	std::string &ob = outbuff[fd];
	
	while (!ob.empty()){
		ssize_t w = ::send(fd, ob.data(), ob.size(), 0);
		if (w > 0) {
			ob.erase(0, static_cast<size_t>(w));// remove sent data
		} else if (w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			return false; // cannot send more now (对方 TCP 接收窗口满了、内核发送缓冲不够等)
		} else {
			std::perror("send");
			cleanupIndex(i);
			return true;//index is removed
		}
	}
	pollfds[i].events = POLLIN;//all sent
	return false;
}

void Server::pushLine(int fd, const std::string& msg) {
    std::map<int, std::string>::iterator it = outbuff.find(fd);
    if (it == outbuff.end()) return;
    it->second.append(msg);
    // ensure POLLOUT is set on this fd
    for (size_t i = 1; i < pollfds.size(); ++i) {
        if (pollfds[i].fd == fd) {
            pollfds[i].events |= POLLOUT;
            break;
        }
    }
}


//unfinished
void Server::sendNumeric(int fd, const std::string& code, const std::string& p1, const std::string& msg) {
    std::string line;
    line.reserve(64 + code.size() + p1.size() + msg.size());
    line += code;
    if (!p1.empty()) { line += " "; line += p1; }
    if (!msg.empty()) { line += " :"; line += msg; }
    line += "\r\n";
    pushLine(fd, line);
}

void Server::pushToChannel(Channel& ch, const std::string& line, int exceptFd) {
    // Assumes Channel exposes: const std::vector<int>& members() const;
    const std::vector<int>& mem = ch.members();
    for (size_t i = 0; i < mem.size(); ++i) {
        int mfd = mem[i];
        if (mfd == exceptFd) continue;
        pushLine(mfd, line);
    }
}