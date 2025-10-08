/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 23:56:57 by junjun            #+#    #+#             */
/*   Updated: 2025/10/08 18:56:43 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP


#include <string>
#include <set>
#include <map> //C++98-friendly (uses std::map/std::set, no std::unordered_map & std::unordered_set from C++11)

class Client; // forward-declare to avoid circular includes

class Channel {
private:
	std::string chan_name;
	std::string chan_topic;
	std::string chan_passkey; // +k mode 
	bool inviteOnly; // +i mode 
	bool topicRestriction; // +t mode 
	size_t maxUserLimit; // +l (0 => unlimited)

	std::map<std::string, Client*> members; // nickname -> Client*
	std::map<std::string, Client*> operators; // nickname -> Client*
	std::set<std::string> invitedUsers; // for +i

	// helpers
	void sendNamesList_(Client* to);
	void sendTopicIfAny_(Client* to);
	
public:
	explicit Channel(const std::string& name);
	Channel(const Channel& other); 
	Channel& operator=(const Channel& other); 
	~Channel();

	//getters
	const std::string& getName() const { return chan_name; }
	const std::string& getTopic() const { return chan_topic; }
	const std::string getPasskey() const { return chan_passkey; }
	size_t getUserLimit() const { return maxUserLimit; }
	bool isTopicRestriction() const { return topicRestriction; }
	int getMemberCount() const { return static_cast<int>(members.size()); }
	
	bool isFull() const { return (maxUserLimit > 0 && members.size() >= maxUserLimit); }
	bool isMember(const std::string& nickname) const  {return members.find(nickname) != members.end();}
	bool isOperator(const std::string& nickname) const { return operators.find(nickname) != operators.end(); }
	bool isInvited(const std::string& nickname) const { return invitedUsers.find(nickname) != invitedUsers.end(); }

	std::string getModesString() const;

	
	//setters
	void setPasskey(const std::string& key) { chan_passkey = key; }
	void setInviteOnly(bool set) { inviteOnly = set; }
	void setTopicRestriction(bool set) { topicRestriction = set; }
	void setUserLimit(size_t limit) { maxUserLimit = limit; }
	void setTopic(const std::string& newTopic, Client* setter);
	void setMode(char mode, bool set, const std::string& param);	
	
	
	//management
	bool addMember(Client* client, const std::string& password);
	void removeMember(const std::string& nickname);
    bool kickMember(Client* requester, const std::string& targetNick, const std::string& reason);
    void addOperator(const std::string& nickname);
    void removeOperator(const std::string& nickname);
	void inviteUser(const std::string& nickname) { invitedUsers.insert(nickname); }
	
	
	//broadcast
	void broadcast(const std::string& msg, Client* exclude);
	
	//authorization check
	bool canChangeTopic(const std::string& nickname) const;
	bool canJoin(Client* client, const std::string& password);
	

};
#endif // CHANEL_HPP