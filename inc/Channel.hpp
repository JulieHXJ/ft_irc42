/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmonika <mmonika@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 23:56:57 by mmonika           #+#    #+#             */
/*   Updated: 2025/11/09 21:06:40 by mmonika          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include "Global.hpp"
#include "Logger.hpp"
#include <string>
#include <set>
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
// if needed we will convert to std::map and std::set
    	std::unordered_map<std::string, Client*>	members;		// All channel members
    	std::unordered_set<std::string>				operators;      // Channel operators only
		std::unordered_set<std::string>				invitedUsers;	// For +i mode

	public:
// if needed will use explicit Channel
		Channel(const std::string& name);
		Channel(const Channel& other);
		Channel& operator=(const Channel& other);
		~Channel();

		// Getters
    	std::string getName() const; //const std::string& getName() const;
    	std::string getTopic() const; //const std::string& getTopic() const;
    	int			getMemberCount() const;
    	bool		isFull() const;
    
		// Member management
    	bool addMember(Client* client, const std::string& password);
    	void removeMember(const std::string& nickname);
    	bool isMember(const std::string& nickname) const;
    
    	// Operator management
    	bool isOperator(const std::string& nickname) const;
    	bool addOperator(const std::string& nickname);
    	bool removeOperator(const std::string& nickname);
    
	    // Invite management
    	bool inviteUser(const std::string& nickname);
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
		void sendNamesList(Client* client) const;
		void sendTopic(Client* client);
};
