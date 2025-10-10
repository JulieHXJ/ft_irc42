/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 20:06:12 by junjun            #+#    #+#             */
/*   Updated: 2025/10/10 18:26:53 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include "../inc/Log.hpp" // nowStr(), newConnect()

extern volatile sig_atomic_t g_stop;// define in main (“There is a variable called g_stop, it’s declared elsewhere, it can change suddenly (signal), and I want to use it here.”)


Server::~Server(){
	for (size_t i = 0; i < pollfds.size(); ++i) {
		::close(pollfds[i].fd);
    }
    pollfds.clear();
	std::map<std::string, Channel*>::iterator it = channel_lst.begin();
	while (it != channel_lst.end()) {
		Channel* toDelete = it->second;
		std::map<std::string, Channel*>::iterator er = it++;
		channel_lst.erase(er);
		delete toDelete;
	}
	client_lst.clear();//why at last?
}

Server::Server(const Server& other) {
	// Copy configuration, but not open resources
	this->listenfd = -1;
	this->password = other.password;
	this->pollfds.clear();
	this->client_lst.clear();
	this->channel_lst.clear();
}

Server& Server::operator=(const Server& rhs){
    if (this == &rhs) return *this;
    for (size_t i = 0; i < pollfds.size(); ++i) {
        if (pollfds[i].fd >= 0) ::close(pollfds[i].fd);
    }
    pollfds.clear();
    std::map<std::string, Channel*>::iterator it = channel_lst.begin();
    while (it != channel_lst.end()) {
        Channel* toDelete = it->second;
        std::map<std::string, Channel*>::iterator er = it++;
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
	if (pfds.empty() || index >= pfds.size()) return;
    pfds[index] = pfds[ pfds.size() - 1 ];  // copy last element into index
    pfds.pop_back();   
}


/**
 * @brief Close the socket, erase buffers, and remove from pollfds.
 */
void Server::cleanupIndex(size_t i){
    if (i >= pollfds.size()) return;
    int fd = pollfds[i].fd;

	//remove from pollfd_list, all channels and client list
	removePollFd(pollfds, i);
	removeClientFromAllChannels(fd);
	client_lst.erase(fd);
	Log::closed(fd);
	// finally close the socket
	::close(fd);
}

void Server::removeClientFromAllChannels(int fd) {
	std::map<int, Client>::iterator client_it = client_lst.find(fd);
	if (client_it == client_lst.end()) return;
	Client& c = client_it->second;
	const std::string nick = c.getNickname();
	std::map<std::string, Channel*>::iterator it = channel_lst.begin(); 
	while (it != channel_lst.end()) {
		Channel* channel = it->second;
		if (channel->isMember(nick)) {
			channel->broadcastInChan(":" + nick + " QUIT :Client disconnected", 0);
			channel->removeMember(nick);
		}
		//delete empty channel
		if (channel->getMemberCount() == 0) {
			Channel* toDelete = channel;
			std::map<std::string, Channel*>::iterator er = it++;
			channel_lst.erase(er);
			delete toDelete;
		} else { ++it; }
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
        addPollFd(pollfds, connfd, POLLIN);
		Client* newClient = new Client(connfd);
		newClient->detectHostname();
		client_lst[connfd] = newClient;

        // send welcome message
		newClient->sendMessage(":" SERVER_NAME RPL_WELCOME " NOTICE * :🎉🎉 Yo! Welcome to *Club42 Chatroom* 🍹");
		newClient->sendMessage(": NOTICE * :*** Looking up your hostname...");
        newClient->sendMessage(": NOTICE * :*** Found your hostname");
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




	
	std::map<int, Client>::iterator it = client_lst.find(fd);
	if (it == client_lst.end()) { cleanupIndex(i); return true; }
	Client& cl = it->second;

    char buf[4096];
	ssize_t r = ::recv(fd, buf, sizeof(buf), 0);
	if (r == 0) {
		//client closed connection
		Log::closed(fd);
		cleanupIndex(i);
		return true;
	} else if (r < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) return false; // no data available
		std::perror("recv");
		cleanupIndex(i);
		return true;
	}
	cl.appendInbuff(buf);//append received data to client inbuff
	Log::recvBytes(fd, static_cast<size_t>(r));
	
	// 2) pop a complete line from client inbuff and handle commands
	std::string Line;
	while (cl.extractLine(Line)) {
		handleCmd(cl, Line); // parse and handle cmds
	}

    // If output is not empty (from WHO/welcome), keep POLLOUT on
    if (cl.hasOutput()) {
        pollfds[i].events |= POLLOUT;
	}
	return false; // not removed
}

bool Server::handleWritable(size_t i){
	int fd = pollfds[i].fd;
	Client* cl = client_lst[fd]


	
	std::map<int, Client>::iterator it = client_lst.find(fd);
	if (it == client_lst.end()) {
		cleanupIndex(i);
		return true;//index is removed
	}
	
	std::string &ob = it->second.getOutput();
	if (ob.empty()){
		pollfds[i].events &= ~POLLOUT; // only close POLLOUT
		return false;
	}
	
	ssize_t n = ::send(fd, ob.data(), ob.size(), 0);
	if (n < 0){
		if (errno == EAGAIN || errno == EWOULDBLOCK) return false;
        cleanupIndex(i);
        return true;//finished sending
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
    std::map<int, Client>::iterator it = client_lst.find(fd);
    if (it == client_lst.end()) return;
   	it->second.appendInbuff(msg + CRLF);

    // turn on POLLOUT
    for (size_t i=0; i<pollfds.size(); ++i) {
        if (pollfds[i].fd == fd) {
            pollfds[i].events |= POLLOUT;
            break;
        }
    }
}
