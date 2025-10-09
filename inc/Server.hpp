/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 20:01:40 by junjun            #+#    #+#             */
/*   Updated: 2025/10/09 23:41:30 by junjun           ###   ########.fr       */
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
	std::vector<struct pollfd> pollfds;     // [0] is listenfd; others are clients
	ClientMap             client_lst;   // fd -> Client*(where includes inbuff and outbuff)
	ChannelMap             channel_lst; 

	//server helper functions
	void setNonBlocking(int fd);
	void addPollFd(std::vector<pollfd>& pfds, int fd, short events);
	void removePollFd(std::vector<pollfd>& pfds, size_t index);

	//server main loop modules
	void acceptNew();
	void cleanupIndex(size_t i);
	bool handleReadable(size_t i);
	bool handleWritable(size_t i);

	// Command handlers (defined in Server_cmd.cpp)
	void handleLine(int fd, const std::string& rawLine);
	void handlePASS(int fd, const std::string& pass);

	// channel & client helpers
	void removeClientFromAllChannels(int fd);
	
public:
	Server() : listenfd(-1) {}
    ~Server();
    Server(const Server& other);              // 拷贝构造：仅复制配置，不复制打开资源
    Server& operator=(const Server& rhs); 

	void serverInit(int port, std::string password);//setup server socket，listen， add to pollfd list
	void run(); //main loop: accept, recv, send 

	// privmsg helpers
    void pushToClient(int fd, const std::string& msg);
	
};

#endif // SERVER_HPP