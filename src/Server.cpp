/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 20:06:12 by junjun            #+#    #+#             */
/*   Updated: 2025/10/17 13:32:40 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include "../inc/Log.hpp" // nowStr(), newConnect()

extern volatile sig_atomic_t g_stop;// define in main (“There is a variable called g_stop, it’s declared elsewhere, it can change suddenly (signal), and I want to use it here.”)


Server::Server(const Server& other) {
	// Copy configuration, but not open resources
	this->listenfd = -1;
	this->password = other.password;
	this->pollfds.clear();
	this->client_lst.clear();
	this->channel_lst.clear();
}

Server::~Server(){
	for (size_t i = 0; i < pollfds.size(); ++i) {
		::close(pollfds[i].fd);
	}
	pollfds.clear();

	 // delete all channels
	for (ChannelMap::iterator it = channel_lst.begin(); it != channel_lst.end(); ++it) {
		delete it->second;
	}
	channel_lst.clear();

	// delete all clients
	for (ClientMap::iterator it = client_lst.begin(); it != client_lst.end(); ++it) {
		delete it->second;
	}
	client_lst.clear();//why at last?
}

Server& Server::operator=(const Server& rhs){
    if (this == &rhs) return *this;
    for (size_t i = 0; i < pollfds.size(); ++i) {
        if (pollfds[i].fd >= 0) ::close(pollfds[i].fd);
    }
    pollfds.clear();
    ChannelMap::iterator it = channel_lst.begin();
    while (it != channel_lst.end()) {
        Channel* toDelete = it->second;
        ChannelMap::iterator er = it++;
        channel_lst.erase(er);
        delete toDelete;
    }
    client_lst.clear();

    listenfd = -1;
    password = rhs.password;
    return *this;
}

//helpers
void Server::setNonBlocking(int fd){
	int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1) flags = 0;
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error(std::string("fcntl(O_NONBLOCK): ") + std::strerror(errno));
	}
	// #ifdef __APPLE__
	// int set = 1;
	// setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));
	// #endif

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
	fd_index[sckfd] = pfds.size() - 1; // map fd to its index in pollfds
}

/**
 * @brief Remove a fd from pollfd list by index
 * @note swap with the last one and pop_back
 */
void Server::removePollFd(std::vector<pollfd>& pfds, size_t index) {
	if (pfds.empty() || index >= pfds.size()) return;

	const int removed_fd = pfds[index].fd;
    if (index != pfds.size() - 1) {
        const int last = pfds[pfds.size() - 1].fd;
        pfds[index] = pfds[pfds.size() - 1];
        fd_index[last] = index;
    }
	pfds.pop_back();
	fd_index.erase(removed_fd); // remove the erased fd from map
}


/**
 * @brief Close the socket, erase buffers, and remove from pollfds.
 */
void Server::cleanupIndex(size_t i){
    if (i >= pollfds.size()) return;
    const int fd = pollfds[i].fd;

	removeClientFromAllChannels(fd);
    ClientMap::iterator it = client_lst.find(fd);
    if (it != client_lst.end()) {
        delete it->second;
        client_lst.erase(it);
    }
	
	removePollFd(pollfds, i);
	Log::closed(fd);
	::close(fd);
}

void Server::removeClientFromAllChannels(int fd) {
	ClientMap::iterator cit = client_lst.find(fd);
	if (cit == client_lst.end()) return;
	Client* c = cit->second;
	const std::string nick = c ? c->getNickname() : "";
	
	for (ChannelMap::iterator chIt = channel_lst.begin(); chIt != channel_lst.end();) {
		Channel* channel = chIt->second;
		if (channel && !nick.empty() && channel->isMember(nick)) {
			channel->broadcastInChan(":" + nick + " QUIT :Client disconnected", 0);
			channel->removeMember(nick);
		}
		//delete empty channel
		if (channel && channel->getMemberCount() == 0) {
			Channel* toDelete = channel;
			ChannelMap::iterator er = chIt++;
			channel_lst.erase(er);
			delete toDelete;
		} else { ++chIt; }
	}
}

/* ============================ server functions ============================ */
void Server::serverInit(int port, std::string password){
	this->password = password;
	//1) Create socket. ipv4, tcp
	listenfd = ::socket(AF_INET, SOCK_STREAM, 0); 
	if (listenfd < 0) { 
		throw std::runtime_error("socket failed");
	} 
	//2) set socket options
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
        throw std::runtime_error(std::string("bind failed: ")+ std::strerror(errno));
    }
	// 4) listen
    if (::listen(listenfd, 64) < 0) {
		::close(listenfd);
        throw std::runtime_error(std::string("listen failed: ") + std::strerror(errno));
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
	
		//1. accept new connections: create client, add to client_lst
		acceptNewConnect();
		
		//2. handle each client socket， increment i only if not removed
		for (size_t i = 1; i < pollfds.size();)
		{
			short re = pollfds[i].revents;
			bool removed = false;

			//2.1 Errors/Hangups (POLLERR | POLLHUP | POLLNVAL)
			if (re & (POLLERR | POLLHUP | POLLNVAL)) {
				cleanupIndex(i);
				removed = true;
			}
			
			//2.2 handle readable fds
			if (!removed && (re & POLLIN)) {
				removed = handleClientRead(i);
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
void Server::acceptNewConnect(){
    if (pollfds.empty() || !(pollfds[0].revents & POLLIN)) return;//no new connection

    for(;;){
        sockaddr_in clientAddr; socklen_t clientLen = sizeof(clientAddr);
        int connfd = ::accept(listenfd, (sockaddr*)&clientAddr, &clientLen);
        if (connfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            if (errno == EINTR) continue;
            perror("accept"); 
			break;
        }
		setNonBlocking(connfd);
        addPollFd(pollfds, connfd, POLLIN | POLLOUT);
		
		Client* newClient = new Client(connfd);
		newClient->detectHostname();
		client_lst[connfd] = newClient;
        newClient->sendMessage(": NOTICE * :*** Enter your PASS, NICK, and USER u 0 * :real to complete registeration");
		
        // enable write for the newly added (it's at the back)
        pollfds[ pollfds.size() - 1 ].events |= POLLOUT;
		
		//put in server log
        Log::newConnect(connfd, clientAddr);
    }
}

/**
 * @brief keep receiving and appending until EAGAIN,
 * then extract lines, process command, generate responses
 */
bool Server::handleClientRead(size_t i){
	int fd = pollfds[i].fd;
	ClientMap::iterator it = client_lst.find(fd);
	if (it == client_lst.end()) { cleanupIndex(i); return true; }
	Client* cl = it->second;

    char buf[4096];
	ssize_t r = ::recv(fd, buf, sizeof(buf), 0);
	if (r == 0) {
		Log::closed(fd);
		cleanupIndex(i);
		return true;
	} else if (r < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) return false; // no data available
		std::perror("recv");
		cleanupIndex(i);
		return true;
	}
	cl->appendInbuff(buf, (size_t)r);//append received data to client inbuff
	Log::recvBytes(fd, static_cast<size_t>(r));
	
	// 2) pop a complete line from client inbuff and handle commands
	std::string line;
	while (cl->extractLine(line)) {
		Log::dbg("fd=" + std::to_string(fd) + " CMD: [" + line + "]");//for debug
		handleCmd(cl, line);
	}

    // If output is not empty, keep POLLOUT on
    if (cl->hasOutput()) {
        pollfds[i].events |= POLLOUT;
	}
	return false;
}

bool Server::handleWritable(size_t i){
	int fd = pollfds[i].fd;
	ClientMap::iterator it = client_lst.find(fd);
	if (it == client_lst.end()) {
		cleanupIndex(i);
		return true;
	}
	Client* cl = it->second;
	std::string &ob = cl->getOutput();
	if (ob.empty()){
		pollfds[i].events &= ~POLLOUT; // only close POLLOUT
		return false;
	}
	
	ssize_t n = ::send(fd, ob.data(), ob.size(), 0);
	if (n < 0){
		if (errno == EAGAIN || errno == EWOULDBLOCK) return false;
        cleanupIndex(i);
        return true;
	}
	Log::sendBytes(fd, static_cast<size_t>(n));
	ob.erase(0, static_cast<size_t>(n));// remove sent data
	
	if (ob.empty()) {
		pollfds[i].events &= ~POLLOUT; // all sent, disable POLLOUT
	}
	return false;
}

/**
 * @brief Append a message (with CRLF) to the client's output buffer and enable POLLOUT.
 */
void Server::pushToClient(int fd, const std::string& msg) {
    ClientMap::iterator it = client_lst.find(fd);
    if (it == client_lst.end()) {
		Log::warn("pushToClient: fd not found: " + std::to_string(fd));
		return;
	}
	Client* cl = it->second;
   	cl->sendMessage(msg);
    // turn on POLLOUT change to fd_index
    for (size_t i = 0; i < pollfds.size(); ++i) {
        if (pollfds[i].fd == fd) {
            pollfds[i].events |= POLLOUT;
            break;
        }
    }
}

void Server::sendWelcome(Client* c) {
    if (!c) return;
    const std::string nick = c->getNickname().empty() ? "*" : c->getNickname();
    c->sendMessage(":" SERVER_NAME " " RPL_WELCOME  " " + nick + " :Welcome to " SERVER_NAME ", " + nick);
    c->sendMessage(":" SERVER_NAME " " RPL_YOURHOST " " + nick + " :Your host is " + c->getHostname() + ", running version v0.1 o itkol");
    c->sendMessage(":" SERVER_NAME " " RPL_CREATED  " " + nick + " :This server was created just now");
}
