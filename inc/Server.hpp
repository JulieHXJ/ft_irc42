/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 20:01:40 by junjun            #+#    #+#             */
/*   Updated: 2025/10/10 18:45:50 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "../inc/Global.hpp"
#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"
#include <unistd.h> // close
#include <sys/socket.h> // socket, bind, listen, accept, recv, send
#include <netinet/in.h> // sockaddr_in
#include <poll.h>

#include <fcntl.h>
#include <arpa/inet.h> // inet_ntop
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sstream>          // std::ostringstream
#include <csignal>

class Server {
private:
	int listenfd;                           // listening socket
	std::string password;
	std::vector<struct pollfd> 	pollfds;     // [0] is listenfd; others are clients
	ClientMap             		client_lst;   // fd -> Client*(where includes inbuff and outbuff)
	ChannelMap            		channel_lst; 

	//server helper functions
	void setNonBlocking(int fd);
	void addPollFd(std::vector<pollfd>& pfds, int fd, short events);
	void removePollFd(std::vector<pollfd>& pfds, size_t index);

	//server main loop modules
	void acceptNewConnect();//detection on listenfd, accept, create client(fd, host), add to pollfd list
	void cleanupIndex(size_t i);
	bool handleClientRead(size_t i);
	bool handleWritable(size_t i);

	// Command handlers (defined in Server_cmd.cpp)
	void handleCmd(Client* c, const std::string& Line);
	void handlePASS(Client* c, const std::vector<std::string>& params);
    void handleNICK(Client* c, const std::vector<std::string>& params);
    void handleUSER(Client* c, const std::vector<std::string>& params);
	void pushToCleint();

	// channel & client helpers
	void removeClientFromAllChannels(int fd);
	
public:
	Server() : listenfd(-1){}
    ~Server();
    Server(const Server& other);       
    Server& operator=(const Server& rhs); 

	void serverInit(int port, std::string password);//setup server socket，listen， add to pollfd list
	void run(); //main loop: accept, recv, send 

	// privmsg helpers
    void pushToClient(int fd, const std::string& msg);
	
};

#endif // SERVER_HPP