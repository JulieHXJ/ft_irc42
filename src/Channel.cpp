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

Channel::Channel(const std::string& name) : name(name), topic(""), passKey(""), inviteOnly(false), topicRestriction(false), maxUserLimit(0) {}

Channel::Channel(const Channel& other) {*this = other;}

Channel& Channel::operator=(const Channel& other) {
    if (this != &other) {
        name = other.name;
        topic = other.topic;
        passKey = other.passKey;
        inviteOnly = other.inviteOnly;
        topicRestriction = other.topicRestriction;
        maxUserLimit = other.maxUserLimit;
        members = other.members;
        operators = other.operators;
        invitedUsers = other.invitedUsers;
    }
    return *this;
}

Channel::~Channel() {
    members.clear();
    operators.clear();
    invitedUsers.clear();
}

std::string Channel::getName() const { return name; }

std::string Channel::getTopic() const { return topic; }

int Channel::getMemberCount() const { return members.size(); }

// bool Channel::isFull() const;

// bool Channel::addMember(Client* client, const std::string& password);

void Channel::removeMember(const std::string& nickname) {
    members.erase(nickname);
    operators.erase(nickname);
}

bool Channel::isMember(const std::string& nickname) const {
    return members.find(nickname) != members.end();
}

bool Channel::isOperator(const std::string& nickname) const {
    return operators.find(nickname) != operators.end();
}

void Channel::addOperator(const std::string& nickname) {
    if (isMember(nickname)) {
        operators.insert(nickname);
    }
}

void Channel::removeOperator(const std::string& nickname) {
    operators.erase(nickname);
}

void Channel::inviteUser(const std::string& nickname) {
    invitedUsers.insert(nickname);
}

bool Channel::isInvited(const std::string& nickname) const {
    return invitedUsers.find(nickname) != invitedUsers.end();
}

// void Channel::broadcast(const std::string &msg, Client* exclude);

// void Channel::setMode(char mode, bool set, const std::string& param);

std::string Channel::getModesString() const {
    std::string modes = "+";
    if (inviteOnly) modes += "i";
    if (topicRestriction) modes += "t";
    if (!passKey.empty()) modes += "k";
    if (maxUserLimit > 0) modes += "l";
    return modes;
}

void Channel::setTopic(const std::string& newTopic, Client* setter) { topic = newTopic; }

// bool Channel::canChangeTopic(const std::string& nickname) const;

// bool Channel::kickMember(Client* requester, const std::string& targetNickname, const std::string& reason);

// bool Channel::canJoin(Client* client, const std::string& password);




//     For TOPIC:
// setInviteOnly(bool set) { inviteOnly = set; }
// setTopicRestriction(bool set) { topicRestriction = set; }
// setPassword(const std::string& pass) { passKey = pass; } // for +k, and for -k we set to empty
// setUserLimit(int limit) { maxUserLimit = limit; } // for +l, and for -l we set to 0
// setOperator(const std::string& nickname, bool set) {
//     if (set) {
//         operators.insert(nickname);
//     } else {
//         operators.erase(nickname);
//     }
// }
// bool Channel::addMember(Client* client, const std::string& password) {
//     // Check if the client is already a member?
//     if (members.find(client->getNickname()) != members.end()) {
//         return false; // already in the channel
//     }
//     // Check password if the channel is password protected
//     if (!passKey.empty() && passKey != password) {
//         return false;
//     }
//     // Check invite-only
//     if (inviteOnly) {
//         if (invited.find(client->getNickname()) == invited.end()) {
//             return false;
//         } else {
//             // Remove the invite after use
//             invited.erase(client->getNickname());
//         }
//     }
//     // Check user limit
//     if (maxUserLimit > 0 && members.size() >= maxUserLimit) {
//         return false;
//     }
//     members[client->getNickname()] = client;
//     return true;
// }
// void Channel::kickMember(const std::string& nickname) {
//     removeMember(nickname);
// }
// bool Channel::canSetTopic(const std::string& nickname) const {
//     if (!topicRestriction) {
//         return true;
//     }
//     return isOperator(nickname);
// }