/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 20:01:40 by junjun            #+#    #+#             */
/*   Updated: 2025/09/28 19:21:24 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream> // std::cout, std::cerr
#include <vector>
#include <map>
#include <string> // std::memset, std::strlen, etc.
#include <sys/socket.h> // socket, bind, listen, accept, recv, send
#include <netinet/in.h> // sockaddr_in
#include <poll.h>

#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"

class Server {
private:

	std::map<int, Client>              fd2client;
	std::map<std::string, Client*>     nick2client;
	std::map<std::string, Channel>     channels;

	int listenfd;                           // listening socket
	std::vector<struct pollfd> pollfds;     // [0] is listenfd; others are clients
	std::map<int, std::string> inbuff;      // fd -> input buffer (move to client?)
	std::map<int, std::string> outbuff;     // fd -> output buffer (move to client?)
	
	//helper functions
	static void addPollFd(std::vector<pollfd>& pfds, int fd, short events);
	static void removePollFd(std::vector<pollfd>& pfds, size_t index);
	static bool getLine(std::string& buf, std::string& line);
	static void setNonBlocking(int fd);

	//run() modules
	void acceptNew();//also need to create a client
	void cleanupIndex(size_t i);
	bool handleReadable(size_t i);
	bool handleWritable(size_t i);

	// Command dispatcher
	void handleCommand(int fd, std::string_view line);

	// Send helpers
    void pushLine(int fd, const std::string& msgCRLF);   // enqueue + set POLLOUT
    void sendNumeric(int fd, const std::string& code,
                     const std::string& p1, const std::string& msg);
	
	
public:
	Server(): listenfd(-1){}
	~Server(){closeFds();}  
	
	void serverInit(int port);//setup server socket，listen， add to pollfd list
	void run(); //main loop: accept, recv, send 
	void closeFds();

	void pushToChannel(Channel& ch, const std::string& line, int exceptFd);
	
};

#endif // SERVER_HPP