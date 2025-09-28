/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 23:53:11 by junjun            #+#    #+#             */
/*   Updated: 2025/09/28 16:16:37 by xhuang           ###   ########.fr       */
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
	std::string nick, user, realname, hostname;
	std::string inbox, outbox; // buffers for incoming and outgoing data
	std::set<std::string> channels; // channels the client has joined
public:
	Client(int socketFd): fd(socketFd) {}
	~Client() { if (fd != -1) close(fd); }

	int getFd() const { return fd; }
	std::string& getInbox() { return inbox; }
	std::string& getOutbox() { return outbox; }

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

};

#endif // CLIENT_HPP