/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 18:03:47 by xhuang            #+#    #+#             */
/*   Updated: 2025/10/07 18:21:14 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Channel.hpp"
// If you have a server name macro somewhere
#ifndef SERVERNAME
# define SERVERNAME "irc.local"
#endif

Channel::Channel(const std::string& name)
: name(name)
, topic("")
, passKey("")
, inviteOnly(false)
, topicRestriction(false)
, maxUserLimit(10)
{}

Channel::Channel(const Channel& other) { *this = other; }

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
    // pointers are owned by Server/Client registry, not by Channel
    members.clear();
    operators.clear();
    invitedUsers.clear();
}


std::string Channel::getModesString() const {
    std::string modes = "+";
    if (inviteOnly)    modes += "i";
    if (topicRestriction) modes += "t";
    if (!passKey.empty()) modes += "k";
    if (maxUserLimit > 0) modes += "l";
    return modes;
}

void Channel::setMode(char mode, bool set, const std::string& param) {
    switch (mode) {
        case 'i': inviteOnly = set; break;
        case 't': topicRestriction = set; break;
        case 'k':
            if (set) passKey = param;
            else     passKey.clear();
            break;
        case 'l':
            if (set) {
                // parse positive integer
                size_t n = 0;
                for (size_t i=0;i<param.size();++i) {
                    if (param[i]<'0' || param[i]>'9') { n = 0; break; }
                    n = n*10 + (param[i]-'0');
                }
                maxUserLimit = n;
            } else {
                maxUserLimit = 0;
            }
            break;
        default: /* ignore unknown */ break;
    }
}

void Channel::setTopic(const std::string& newTopic, Client* setter) {
    const std::string& nick = setter->getNickname();
    if (!canChangeTopic(nick)) {
        setter->sendMessage("482 " + nick + " " + name + " :You're not channel operator");
        return;
    }
    topic = newTopic;

    // Broadcast TOPIC
    std::string line = ":" + nick + " TOPIC " + name + " :" + topic;
    broadcast(line, NULL);
}

//channel management
bool Channel::addMember(Client* client, const std::string& password) {
    if (!canJoin(client, password)) return false;

    members[client->getNickname()] = client;
    invitedUsers.erase(client->getNickname());

    // First member becomes op, per common practice
    if (members.size() == 1) {
        operators.insert(client->getNickname());
    }

    // Broadcast JOIN
    std::string joinMsg = ":" + client->getNickname() + " JOIN " + name;
    broadcast(joinMsg, NULL);

    // Send topic numeric if any
    if (!topic.empty()) {
        client->sendMessage(":" + std::string(SERVERNAME) + " 332 " +
                            client->getNickname() + " " + name + " :" + topic);
    }
    // Optionally send names list (RPLNAMREPLY/366) later
    return true;
}

static void Channel::removeMember(const std::string& nickname) {
    std::map<std::string, Client*>::iterator it = members.find(nickname);
    if (it != members.end()) {
        members.erase(it);
    }
    operators.erase(nickname);
}

bool Channel::kickMember(Client* requester, const std::string& targetNickname, const std::string& reason) {
    const std::string reqNick = requester->getNickname();
    if (!isOperator(reqNick)) {
        requester->sendMessage("482 " + reqNick + " " + name + " :You're not channel operator");
        return false;
    }
    if (!isMember(targetNickname)) {
        requester->sendMessage("441 " + reqNick + " " + targetNickname + " " + name + " :They aren't on that channel");
        return false;
    }
    // Broadcast KICK
    std::string line = ":" + reqNick + " KICK " + name + " " + targetNickname + " :" + (reason.empty() ? "Kicked" : reason);
    broadcast(line, NULL);

    removeMember(targetNickname);
    return true;
}

void Channel::addOperator(const std::string& nickname) {
    if (isMember(nickname)) operators.insert(nickname);
}

void Channel::removeOperator(const std::string& nickname) {
    std::set<std::string>::iterator it = operators.find(nickname);
    if (it != operators.end()) operators.erase(it);
}

void Channel::inviteUser(const std::string& nickname) {
    invitedUsers.insert(nickname);
}






bool Channel::canJoin(Client* client, const std::string& password) {
    const std::string nick = client->getNickname();

    if (isMember(nick)) {
        client->sendMessage("443 " + nick + " " + name + " :is already on channel");
        return false;
    }
    if (!passKey.empty() && passKey != password) {
        client->sendMessage("475 " + nick + " " + name + " :Cannot join channel (+k)");
        return false;
    }
    if (inviteOnly && !isInvited(nick)) {
        client->sendMessage("473 " + nick + " " + name + " :Cannot join channel (+i)");
        return false;
    }
    if (isFull()) {
        client->sendMessage("471 " + nick + " " + name + " :Cannot join channel (+l)");
        return false;
    }
    return true;
}



void Channel::broadcast(const std::string& msg, Client* exclude) {
    // msg should already be a full IRC line (with command/prefix), add CRLF here:
    const std::string wire = msg + "\r\n";
    std::map<std::string, Client*>::iterator it = members.begin();
    for (; it != members.end(); ++it) {
        Client* c = it->second;
        if (!c) continue;
        if (exclude && c == exclude) continue;
        c->sendMessage(wire);
    }
}

bool Channel::canChangeTopic(const std::string& nickname) const {
    if (!topicRestriction) return true;
    return isOperator(nickname);
}


