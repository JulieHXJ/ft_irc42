/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Chanel.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 23:56:57 by junjun            #+#    #+#             */
/*   Updated: 2025/09/28 16:23:06 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANEL_HPP
#define CHANEL_HPP

#include <string>
#include <set>


struct ChanModes {
    bool t{false};                       // topic protected
    bool i{false};                       // invite-only
    std::optional<std::string> key;      // +k
    std::optional<size_t>      limit;    // +l
};

class Channel {
private:
	std::string name;
	std::string topic;
	size_t maxUsers;
	size_t currentUsers;
	ChanModes modes;

	std::set<int> members; // set of client fds
	std::set<int> operators; // set of operator fds
public:
	Chanel(const std::string& channelName, size_t maxUsers = 100)
		: name(channelName), topic(""), maxUsers(maxUsers), currentUsers(0) {}

	bool addMember(int fd) {
    if (modes.limit && members.size() >= *modes.limit) return false;
    return members.insert(fd).second;
	}
	void removeMember(int fd) {
		members.erase(fd); operators.erase(fd);
	}
	bool isMember(int fd) const { return members.count(fd); }
	bool isOp(int fd) const     { return operators.count(fd); }

	template<class F>
	void forEachMember(F f) const { for (int m : members) f(m); }

};
#endif // CHANEL_HPP