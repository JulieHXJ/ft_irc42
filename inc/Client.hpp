/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 23:53:11 by junjun            #+#    #+#             */
/*   Updated: 2025/10/14 17:54:49 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../inc/Global.hpp"
#include "../inc/Parser.hpp"
#include <unistd.h> // close
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Server; // forward declare
class Client {
private:
	int fd;
	bool pass_ok;
	bool registered; // whether the client has completed registration
	std::string nickname, username, hostname;
	std::string inbuff, outbuff; // buffers for incoming and outgoing data
	std::vector<std::string> joinedChannels; // channels the client has joined

	// avoiding fd 被多次 close）
    Client(const Client &);
    Client &operator=(const Client &);
	// 查找频道名在 vector 中的下标；找不到返回 npos
    size_t findChannelIndex(const std::string& channel) const;

public:
	explicit Client(int socketFd): fd(socketFd), pass_ok(false), registered(false) {}
	~Client() { if (fd != -1) ::close(fd); }

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

	 // 让 Server::handleWritable 取到可变的输出缓冲
    std::string& getOutput() { return outbuff; }//why?

	//setters
	void setFd(int socketFd) { fd = socketFd; }
	void setPassOk(const std::string& password) { (void)password; this->pass_ok = true; }
	void setNickname(const std::string& nick) { this->nickname = nick; }
	void setUsername(const std::string& username) { this->username = username; }
	void setRegistered();
	
	// I/P helpers
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