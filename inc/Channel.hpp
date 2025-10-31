/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 23:56:57 by junjun            #+#    #+#             */
/*   Updated: 2025/11/01 01:09:34 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include "Global.hpp"

#include <string>
#include <unordered_set>
#include <unordered_map>

class Client;

class Channel {
	private:
    	std::string	name;
    	std::string	topic;
    	std::string	passKey;			// +k mode
    	bool		inviteOnly;			// +i mode
    	bool		topicRestriction;	// +t mode
    	size_t		maxUserLimit;		// +l mode

    	std::unordered_map<std::string, Client*>	members;		// All channel members
    	std::unordered_map<std::string, Client*>	operators;      // Channel operators only
		std::unordered_set<std::string>				invitedUsers;	// For +i mode

	public:
		Channel(const std::string& name);
		Channel(const Channel& other);
		Channel& operator=(const Channel& other);
		~Channel();

		// Getters
    	std::string getName() const;
    	std::string getTopic() const;
    	int			getMemberCount() const;
    	bool		isFull() const;
    
		// Member management
    	bool addMember(Client* client, const std::string& password);
    	void removeMember(const std::string& nickname);
    	bool isMember(const std::string& nickname) const;
    
    	// Operator management
    	bool isOperator(const std::string& nickname) const;
    	void addOperator(const std::string& nickname);
    	void removeOperator(const std::string& nickname);
    
	    // Invite management
    	void inviteUser(const std::string& nickname);
    	bool isInvited(const std::string& nickname) const;
    
    	// Message broadcasting
		void broadcast(const std::string &msg, Client* exclude);
    
	    // Mode management
    	void setMode(char mode, bool set, const std::string& param);
    	std::string getModesString() const;
    
    	// Topic management
    	void setTopic(const std::string& newTopic, Client* client);
    	bool canChangeTopic(const std::string& nickname) const;
		
		//Kick command
		bool kickMember(Client* client, const std::string& targetNickname, const std::string& reason);
    
		// Validation methods
    	bool canJoin(Client* client, const std::string& password);
		void Channel::sendNamesList(Client* client) const;
		// void setOperatorPrivilege(const std::string& nickname, bool isOperator);
};
