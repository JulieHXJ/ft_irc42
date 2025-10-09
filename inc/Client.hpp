/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 23:53:11 by junjun            #+#    #+#             */
/*   Updated: 2025/10/09 23:44:13 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../inc/Global.hpp"
#include <unistd.h> // close
#include <set>

class Client {
private:
	int fd;
	bool pass_ok;
	bool registered; // whether the client has completed registration
	std::string nickname, username, host;
	std::string inbuff, outbuff; // buffers for incoming and outgoing data
	std::set<std::string> joinedChannels; // channels the client has joined
public:
	Client() : fd(-1), pass_ok(false), registered(false) {}
	Client(int socketFd): fd(socketFd), pass_ok(false), registered(false) {}
	~Client() { if (fd != -1) close(fd); }

	//getters
	int getFd() const { return fd; }
	bool isPassOk() const { return pass_ok; }
	bool isRegistered() const { return registered; }
	const std::string& getNickname() const { return nickname; }
	const std::string& getUsername() const { return username; }
	std::string& getInput() { return inbuff; }//for server to read
	std::string& getOutput() { return outbuff; }//for server to write

	//setters
	void setFd(int socketFd) { fd = socketFd; }
	void setPassOk(bool v) { pass_ok = v; }
	void setRegistered(bool v) { registered = v; }
	void setNickname(const std::string& nickname) { this->nickname = nickname; }
	void setUsername(const std::string& username) { this->username = username; }

	
	void sendMessage(const std::string& message) { outbuff += message + CRLF;}
	
	//pop a complete lines from client inbuff without \r\n
    bool getLine(std::string& line){
		std::string::size_type pos = inbuff.find(CRLF);
		if (pos == std::string::npos) return false;
		line = inbuff.substr(0, pos);//pop out the line without \r\n
		inbuff.erase(0, pos + 2);//remove the line with \r\n
		return true;
	}

	void clearInbox() { inbuff.clear(); }
	void clearOutbox() { outbuff.clear(); }

};

#endif // CLIENT_HPP