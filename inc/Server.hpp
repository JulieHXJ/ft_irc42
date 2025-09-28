/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 20:01:40 by junjun            #+#    #+#             */
/*   Updated: 2025/09/28 15:39:44 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream> // std::cout, std::cerr
#include <vector>
#include <map>
#include <cstring> // std::memset, std::strlen, etc.
#include <sys/socket.h> // socket, bind, listen, accept, recv, send
#include <netinet/in.h> // sockaddr_in
#include <poll.h>
// #include "../inc/Client.hpp"

class Server {
private:
	int listenfd;                           // listening socket
	std::vector<struct pollfd> pollfds;     // [0] is listenfd; others are clients
	std::map<int, std::string> inbuff;      // fd -> input buffer
	std::map<int, std::string> outbuff;     // fd -> output buffer
	
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
	
	
public:
	Server(): listenfd(-1){}
	~Server(){closeFds();}  
	
	void serverInit(int port);//setup server socket，listen， add to pollfd list
	void run(); //main loop: accept, recv, send 
	void closeFds();

};

#endif // SERVER_HPP