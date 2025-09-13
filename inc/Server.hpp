/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 20:01:40 by junjun            #+#    #+#             */
/*   Updated: 2025/09/13 21:10:18 by junjun           ###   ########.fr       */
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

class Server {
private:
	int listenfd; //listening socket
	// int connfd; //connection socket for a client
	std::vector<struct pollfd> pollfds; //for poll()
	std::map<int, std::string> clientBuffer; //map fd to client data buffer

	//helper functions
	static void addPollFd(std::vector<pollfd>& pfds, int fd, short events);
	static void removePollFd(std::vector<pollfd>& pfds, size_t index);
	
public:
	Server(): listenfd(-1){}
	~Server(){closeFds();}  
	
	void serverInit(int port);//setup server socket，listen， add to pollfd list
	void run(); //main loop: accept, recv, send 

	static void setNonBlocking(int fd);
	void closeFds();
};

#endif // SERVER_HPP