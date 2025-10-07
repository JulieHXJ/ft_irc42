/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 20:01:40 by junjun            #+#    #+#             */
/*   Updated: 2025/10/07 18:44:56 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#pragma once
#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"
#include <unistd.h> // close
#include <iostream> // std::cout, std::cerr
#include <vector>
#include <map>
#include <string> // std::memset, std::strlen, etc.
#include <sys/socket.h> // socket, bind, listen, accept, recv, send
#include <netinet/in.h> // sockaddr_in
#include <poll.h>

#ifndef SERVER_NAME
# define SERVER_NAME "irc.local"
#endif

class Server {
private:
	int listenfd;                           // listening socket
	std::vector<struct pollfd> pollfds;     // [0] is listenfd; others are clients
	std::map<int, std::string> inbuff;      // fd -> input buffer (move to client?)
	std::map<int, std::string> outbuff;     // fd -> output buffer (move to client?)
	
	std::map<int, Client>              client_lst;   // fd -> Client*
	std::map<std::string, Client*>     nick2client;
	std::map<std::string, Channel>     channel_lst; 

	
	//helper functions
	void setNonBlocking(int fd);
	void addPollFd(std::vector<pollfd>& pfds, int fd, short events);
	void removePollFd(std::vector<pollfd>& pfds, size_t index);
	bool getLine(std::string& buf, std::string& line);

	//server main loop modules
	void acceptNew();
	void cleanupIndex(size_t i);
	bool handleReadable(size_t i);
	bool handleWritable(size_t i);

	// Command handlers
	void handleCmd(int fd, const std::string& line);
    // void handleNick(int fd, const std::string& nick);
    // void handleUser(int fd, const std::string& user, const std::string& real);
    // void maybeRegister(Client* c);

	// channel & client helpers
    Channel& newChannel(const std::string& name);
    void joinChannel(int fd, const std::string& name, const std::string& key);
    void partChannel(int fd, const std::string& name, const std::string& reason);
    void removeClientFromAllChannels(int fd);

	// Send helpers
    void sendNumeric(int fd, const std::string& code,
                     const std::string& p1, const std::string& msg);
	
	
public:
	Server(): listenfd(-1){}
	~Server(){closeFds();}  
	
	void serverInit(int port);//setup server socket，listen， add to pollfd list
	void run(); //main loop: accept, recv, send 
	void closeFds();

	// Send helpers
    void pushLine(int fd, const std::string& msgCRLF);   // for client: sendMsg
	
};

#endif // SERVER_HPP