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

bool Channel::isFull() const {
    return (maxUserLimit > 0 && members.size() >= maxUserLimit);
}

bool Channel::addMember(Client* client, const std::string& password)
{
    if (!canJoin(client, password)) {
        return false;
    }
    members[client->getNickname()] = client;
    invitedUsers.erase(client->getNickname());
    if (members.size() == 1) {
        operators[client->getNickname()] = client;
    }
    std::string joinMsg = ":" + client->getNickname() + " JOIN " + name; // :john!johndoe@localhost JOIN #general
    broadcast(joinMsg);
    if (!topic.empty()) {
        client->sendMessage(":" + std::string(SERVER_NAME) + " 332 " + client->getNickname() + " " + name + " :" + topic);
    }
    // sendNamesList(client);  do we need to send name list to the joining client?
    return true;
}

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

// void Channel::setTopic(const std::string& newTopic, Client* setter) { topic = newTopic; }

// bool Channel::canChangeTopic(const std::string& nickname) const;

// bool Channel::kickMember(Client* requester, const std::string& targetNickname, const std::string& reason); -->client->getfd()

bool Channel::canJoin(Client* client, const std::string& password)
{
    if (isMember(client->getNickname())) {
        client->sendMessage("443 " + client->getNickname() + " " + name + " :is already on channel");
        return false;
    }
    if (!passKey.empty() && passKey != password) {
        client->sendMessage("475 " + client->getNickname() + " " + name + " :Cannot join channel (+k)");
        return false;
    }
    if (inviteOnly && !isInvited(client->getNickname())) {
        client->sendMessage("473 " + client->getNickname() + " " + name + " :Cannot join channel (+i)");
        return false;
    }
    if (isFull()) {
        client->sendMessage("471 " + client->getNickname() + " " + name + " :Cannot join channel (+l)");
        return false;
    }
    return true;
}




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

// void Channel::kickMember(const std::string& nickname) {
//     removeMember(nickname);
// }
// bool Channel::canSetTopic(const std::string& nickname) const {
//     if (!topicRestriction) {
//         return true;
//     }
//     return isOperator(nickname);
// }