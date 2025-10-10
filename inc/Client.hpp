/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 23:53:11 by junjun            #+#    #+#             */
/*   Updated: 2025/10/10 18:06:44 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../inc/Global.hpp"
#include "../inc/Parser.hpp"
#include <unistd.h> // close
#include <set>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Client {
private:
	int fd;
	bool pass_ok;
	bool registered; // whether the client has completed registration
	std::string nickname, username, hostname;
	std::string inbuff, outbuff; // buffers for incoming and outgoing data
	std::set<std::string> joinedChannels; // channels the client has joined
public:
	Client(int socketFd): fd(socketFd), pass_ok(false), registered(false) {}
	~Client() { if (fd != -1) close(fd); }

	//getters
	int getFd() const { return fd; }
	bool isPassOk() const { return pass_ok; }
	bool isRegistered() const { return registered; }
	const std::string& getNickname() const { return nickname; }
	const std::string& getUsername() const { return username; }
	const std::string& getHostname() const { return hostname; }
	bool hasOutput() const { return !outbuff.empty(); }
    std::vector<std::string> getChannels() const {
		return joinedChannels.empty() ? std::vector<std::string>() : std::vector<std::string>(joinedChannels.begin(), joinedChannels.end()); 
	}

	//setters
	void setFd(int socketFd) { fd = socketFd; }
	void setPassOk(const std::string& password) { (void)password; this->pass_ok = true; }
	void setNickname(const std::string& nick) { this->nickname = nick; }
	void setUsername(const std::string& username) { this->username = username; }
	void setRegistered();
	
	// message methods
	void detectHostname();
	void appendInbuff(const std::string& data) { inbuff += data; }
    bool extractLine(std::string& line);
	void sendMessage(const std::string& message);//add CRLF, queue to outbuff, enable POLLOUT in server, check buffer 
	
	// Channel management
    void joinChannel(const std::string& channel);
    void leaveChannel(const std::string& channel);
    bool isInChannel(const std::string& channel) const;

};

#endif // CLIENT_HPP