/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/11/01 01:15:22 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */



#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"

Channel::Channel(const std::string& name) : name(name), topic(""), passKey(""), inviteOnly(false), topicRestriction(false), maxUserLimit(0) {}

Channel::Channel(const Channel& other) {*this = other;}

Channel& Channel::operator=(const Channel& other) {
    if (this != &other) {
        chan_name = other.chan_name;
        chan_topic = other.chan_topic;
        chan_passkey = other.chan_passkey;
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

bool Channel::addMember(Client* client, const std::string& password) {
    if (!canJoin(client, password)) {
        return false;
    }
    members[client->getNickname()] = client;
    invitedUsers.erase(client->getNickname());
    if (members.size() == 1) {
        operators[client->getNickname()] = client;
    }
    std::string joinMsg = ":" + client->getNickname() + " JOIN " + name; // :john!johndoe@localhost JOIN #general
    broadcast(joinMsg, nullptr);
    if (!topic.empty()) {
        client->sendMessage(":" + std::string(SERVER_NAME) + " 332 " + client->getNickname() + " " + name + " :" + topic);
    }
    // sendNamesList(client);  //do we need to send name list to the joining client?
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

// auto& member: Each iteration gives us a key-value pair:
// member.first: The nickname (string)
// member.second: The Client object pointer
void Channel::broadcast(const std::string &msg, Client* exclude) {
    for (auto& member : members) {
        if (member.second != exclude) {
            try {
                member.second->sendMessage(msg);
            } catch (const std::exception& e) {
                std::cerr << "Failed to send to " << member.first << ": " << e.what() << std::endl;
            }
        }
    }
}

void Channel::setMode(char mode, bool set, const std::string& param) {
    switch (mode) {
        case 'i':
            inviteOnly = set; break;            
        case 't':
            topicRestriction = set; break;
        case 'k':
            if (set && param.empty()) { return; }
            passKey = set ? param : "";
            break;
        case 'o':
            if (param.empty()) { return; }
            if (!isMember(param)) { return; }
            if (set) { addOperator(param);
            } else { removeOperator(param); }
            break;
        case 'l':
            if (set) {
                if (param.empty()) { return; }
                int limit = std::atoi(param.c_str());
                if (limit <= 0) { return; }
                maxUserLimit = limit;
            } else { maxUserLimit = 0; }
            break;
        default:
            break;
    }
}

std::string Channel::getModesString() const {
    std::string modes = "+";
    if (inviteOnly) modes += "i";
    if (topicRestriction) modes += "t";
    if (!passKey.empty()) modes += "k";
    if (maxUserLimit > 0) modes += "l";
    return modes;
}

void Channel::setTopic(const std::string& newTopic, Client* client) {
    if (!canChangeTopic(client->getNickname())) {
        client->sendMessage("482 " + client->getNickname() + " " + name + " :You're not channel operator");
        return;
    }
    topic = newTopic;
    if (!newTopic.empty()) {
        broadcast(":" + client->getNickname() + " TOPIC " + name + " :" + newTopic);
    }
}

bool Channel::canChangeTopic(const std::string& nickname) const {
    if (!topicRestriction) {
        return true;
    }
    return isOperator(nickname);
}

bool Channel::kickMember(Client* client, const std::string& targetNickname, const std::string& reason) {
    if (!isOperator(client->getNickname())) {
        client->sendMessage("482 " + client->getNickname() + " " + name + " :You're not channel operator");
        return false;
    }
    if (!isMember(targetNickname)) {
        client->sendMessage("441 " + client->getNickname() + " " + targetNickname + " " + name + " :They aren't on that channel");
        return false;
    }
    Client* targetClient = members[targetNickname];
    std::string kickMsg = ":" + client->getNickname() + " KICK " + name + " " + targetNickname;
    if (!reason.empty()) {
        kickMsg += " :" + reason;
    }
    removeMember(targetNickname);
    broadcast(kickMsg);
    targetClient->sendMessage(kickMsg);
    return true;
}

bool Channel::canJoin(Client* client, const std::string& password) {
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

void Channel::sendNamesList(Client* client) const {
    std::string namesList;
    for (auto& member : members) {
        const std::string& nickname = member.first;
        
        if (!namesList.empty()) {
            namesList += " ";
        }
        if (isOperator(nickname)) {
            namesList += "@";
        }
        namesList += nickname;
    }
    client->sendMessage(":ircserver 353 " + client->getNickname() + " = " + name + " :" + namesList);
    client->sendMessage(":ircserver 366 " + client->getNickname() + " " + name + " :End of /NAMES list");
}


// void Channel::sendNamesListToAll() const {
//     for (auto& member : members) {
//         sendNamesList(member.second);
//     }
// }

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
