/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 23:56:57 by junjun            #+#    #+#             */
/*   Updated: 2025/10/07 18:21:40 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANEL_HPP
#define CHANEL_HPP

#pragma once
#include "Client.hpp"

#include <string>
#include <set>
#include <map> //std::unordered_map & std::unordered_set are from C++11


class Channel {
private:
	std::string name;
	std::string topic;
	std::string passKey; // +k mode 
	bool inviteOnly; // +i mode 
	bool topicRestriction; // +t mode 
	size_t maxUserLimit; // +l mode

    std::map<std::string, Client*> members;// members: nickname -> Client*
    std::set<std::string>          operators;// operators: just nicknames
    std::set<std::string>          invitedUsers;// invited nicknames for +i
	
public:
	Channel(const std::string& name); 
	Channel(const Channel& other); 
	Channel& operator=(const Channel& other); 
	~Channel();


	//getters
	std::string getName() const { return name; }
	std::string getTopic() const { return topic; }
	int getMemberCount() const { return members.size(); }
	std::string getPassKey() const { return passKey; }
	bool isMember(const std::string& nickname) const  {return members.find(nickname) != members.end();}
	bool isOperator(const std::string& nickname) const { return operators.find(nickname) != operators.end();}
	bool Channel::isInvited(const std::string& nickname) const { return invitedUsers.find(nickname) != invitedUsers.end(); }

	bool isTopicRestriction() const { return topicRestriction; }
	bool isFull() const { return (maxUserLimit > 0 && members.size() >= maxUserLimit); }
	size_t getUserLimit() const { return maxUserLimit; }
	std::string getModesString() const;


	//setters
	void setTopic(const std::string& newTopic, Client* setter);
	void setPassKey(const std::string& key) { passKey = key; }
	void setInviteOnly(bool set) { inviteOnly = set; }
	void setTopicRestriction(bool set) { topicRestriction = set; }
	void setUserLimit(size_t limit) { maxUserLimit = limit; }
	
	void setMode(char mode, bool set, const std::string& param);	
	

	//management
	bool addMember(Client* client, const std::string& password);
    bool kickMember(Client* requester, const std::string& targetNickname, const std::string& reason);
    void addOperator(const std::string& nickname);
    void removeOperator(const std::string& nickname);

	
	//invite
	void inviteUser(const std::string& nickname) { invitedUsers.insert(nickname); }
	bool isInvited(const std::string& nickname) const { return invitedUsers.count(nickname); }

	//broadcast
	void broadcast(const std::string& msg, Client* exclude);

	
	//authorization
	bool canChangeTopic(const std::string& nickname) const {
		if (!topicRestriction) return true;
		return isOperator(nickname);
	}
	bool kickMember(Client* requester, const std::string& targetNickname, const std::string& reason);
	bool canJoin(Client* client, const std::string& password);

};
#endif // CHANEL_HPP