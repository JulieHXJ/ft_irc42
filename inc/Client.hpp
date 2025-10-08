/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 23:53:11 by junjun            #+#    #+#             */
/*   Updated: 2025/10/08 17:35:52 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <unistd.h> // close
#include <set>

class Client {
private:
	int fd{-1};
	bool registered; // whether the client has completed registration
	std::string nick, user, realname, host;
	std::string inbox, outbox; // buffers for incoming and outgoing data
	std::set<std::string> channels; // channels the client has joined
public:
	Client() : fd(-1), registered(false) {}
	Client(int socketFd): fd(socketFd), registered(false) {}
	~Client() { if (fd != -1) close(fd); }

	//getters
	int getFd() const { return fd; }
	std::string& getInbox() { return inbox; }
	std::string& getOutbox() { return outbox; }
	std::string& getNickname() { return nick; }
	

	void appendToInbox(const std::string& data) { inbox += data; }
	void appendToOutbox(const std::string& data) { outbox += data; }

	void clearInbox() { inbox.clear(); }
	void clearOutbox() { outbox.clear(); }


	bool isRegistered() const { return registered; }
	std::string prefix() const {  // ":nick!user@host"
		return ":" + nick + "!" + user + "@" + host;
	}

	void joinChannel(std::string const& name) { channels.insert(name); }
	void leaveChannel(std::string const& name){ channels.erase(name); }
	void sendMessage(const std::string& message) {
		outbox += message + "\r\n"; // IRC messages end with CRLF
	}

};

#endif // CLIENT_HPP