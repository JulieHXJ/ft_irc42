/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmonika <mmonika@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/27 16:16:35 by mmonika           #+#    #+#             */
/*   Updated: 2025/09/27 17:18:55 by mmonika          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Channel.hpp"

Channel(const std::string& name);
~Channel();
std::string getName() const;
std::string getTopic() const;
int			getMemberCount() const;
bool		isFull() const;
// bool addMember(Client* client, const std::string& password = "");
void removeMember(const std::string& nickname);
bool isMember(const std::string& nickname) const;
bool isOperator(const std::string& nickname) const;
void addOperator(const std::string& nickname);
void removeOperator(const std::string& nickname);
void inviteUser(const std::string& nickname);
bool isInvited(const std::string& nickname) const;
// void broadcast(const std::string &msg, Client* exclude = nullptr);
void setMode(char mode, bool set, const std::string& param = "");
std::string getModesString() const;
// void setTopic(const std::string& newTopic, Client* setter);
bool canChangeTopic(const std::string& nickname) const;
// bool kickMember(Client* requester, const std::string& targetNickname, const std::string& reason);
// bool canJoin(Client* client, const std::string& password = "");




//     For TOPIC:
// void setTopic(const std::string& newTopic) { topic = newTopic; }
// std::string getTopic() const { return topic; }
// setInviteOnly(bool set) { inviteOnly = set; }
// setTopicRestriction(bool set) { topicRestriction = set; }
// setPassword(const std::string& pass) { passKey = pass; } // for +k, and for -k we set to empty
// setUserLimit(int limit) { maxUserLimit = limit; } // for +l, and for -l we set to 0

// And for operator mode:
// setOperator(const std::string& nickname, bool set) {
//     if (set) {
//         operators.insert(nickname);
//     } else {
//         operators.erase(nickname);
//     }
// }

// Channel::Channel() : name(""), topic(""), passKey(""), inviteOnly(false), topicRestriction(false), maxUserLimit(0) {}
// Channel(const std::string& name) : name(name), topic(""), passKey(""), inviteOnly(false), topicRestriction(false), maxUserLimit(0) {}
// Channel::~Channel() {
//     members.clear();
//     operators.clear();
//     invited.clear();
// }
// bool Channel::addMember(Client* client, const std::string& password) {
//             // Check if the client is already a member?
// if (members.find(client->getNickname()) != members.end()) {
//     return false; // already in the channel
//     }

//             // Check password if the channel is password protected
//             if (!passKey.empty() && passKey != password) {
//                 return false;
//             }

//             // Check invite-only
//             if (inviteOnly) {
//                 if (invited.find(client->getNickname()) == invited.end()) {
//                     return false;
//                 }
//                 // Remove the invite after use
//                 invited.erase(client->getNickname());
//             }

//             // Check user limit
//             if (maxUserLimit > 0 && members.size() >= maxUserLimit) {
//                 return false;
//             }

//             members[client->getNickname()] = client;
//             return true;
//         }
// void Channel::kickMember(const std::string& nickname) {
//             removeMember(nickname);
//         }

//         void Channel::inviteMember(const std::string& nickname) {
//             invited.insert(nickname);
//         }		
// void Channel::removeMember(const std::string& nickname) {
//             members.erase(nickname);
//             operators.erase(nickname);}
//         bool Channel::isOperator(const std::string& nickname) const {
//             return operators.find(nickname) != operators.end();
//         }
//  bool Channel::canSetTopic(const std::string& nickname) const {
//             if (!topicRestriction) {
//                 return true;
//             }
//             return isOperator(nickname);
//         }