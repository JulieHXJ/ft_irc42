/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 18:03:47 by xhuang            #+#    #+#             */
/*   Updated: 2025/10/08 19:09:46 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Channel.hpp"
#include "../inc/Client.hpp" //provide getNickname() and sendMessage()

#ifndef SERVERNAME
# define SERVERNAME "ircserver"
#endif

Channel::Channel(const std::string& name)
: chan_name(name)
, chan_topic("")
, chan_passkey("")
, inviteOnly(false)
, topicRestriction(false)
, maxUserLimit(0)
{}

Channel::Channel(const Channel& other) { *this = other; }

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
    // pointers are owned by Server/Client registry, not by Channel
    members.clear();
    operators.clear();
    invitedUsers.clear();
}

/* =========== getters and setters ==============*/
void Channel::setTopic(const std::string& newTopic, Client* setter) {
    if (!setter) return;
    const std::string& nick = setter->getNickname();
    if (!canChangeTopic(nick)) {
        setter->sendMessage(":" + std::string(SERVERNAME) + "482 " + nick + " " + chan_name + " :You're not channel operator");
        return;
    }
    chan_topic = newTopic;

    // Broadcast TOPIC :<nick> TOPIC <channel> : <topic>
    std::string line = ":" + nick + "  TOPIC " + chan_name + " : " + chan_topic;
    broadcast(line, NULL);
}

std::string Channel::getModesString() const {
    std::string modes = "+";
    if (inviteOnly)    modes += "i";
    if (topicRestriction) modes += "t";
    if (!chan_passkey.empty()) modes += "k";
    if (maxUserLimit > 0) modes += "l";
    return modes;
}

void Channel::setMode(char mode, bool set, const std::string& param) {
    switch (mode) {
        case 'i': inviteOnly = set; break; // invite only
        case 't': topicRestriction = set; break; //topic restriction
        case 'k':
            if (set) chan_passkey = param;
            else     chan_passkey.clear();
            break;
        case 'o': // add/remove operator
            if (!param.empty()) {
                if (set) addOperator(param);
                else     removeOperator(param);
            }
            break;
        case 'l': // user limit
            if (set) {
                size_t limit = 0;
                for (size_t i=0;i<param.size();++i) {
                    if (param[i] < '0' || param[i] > '9') { limit = 0; break; }
                    limit = limit * 10 + static_cast<size_t>(param[i] - '0');
                }
                maxUserLimit = limit;
            } else {
                maxUserLimit = 0;
            }
            break;
        default: /* ignore unknown */ break;
    }
}

/* =========== membership management ==============*/
bool Channel::addMember(Client* client, const std::string& password) {
    if (!client) return false;
    if (!canJoin(client, password)) return false;
    const std::string nick = client->getNickname();
    members[nick] = client;
    invitedUsers.erase(nick);

    // First member becomes operator
    if (members.size() == 1u) { operators[nick] = client; }

    // Broadcast JOIN: <nick> JOIN <channel>
    const std::string joinMsg = ":" + nick + " JOIN " + chan_name;
    broadcast(joinMsg, NULL);

    // Send topic and names list to the new member
    if (!chan_topic.empty()) {
        client->sendMessage(":" + std::string(SERVERNAME) + " 332 " + nick + " " + chan_name + " :" + chan_topic);
    }
    // Optionally send names list (RPLNAMREPLY/366) later

    // Send topic and names list to the new member
    sendTopicIfAny_(client);//todo
    sendNamesList_(client);//todo
    
    return true;
}

void Channel::removeMember(const std::string& nickname) {
    std::map<std::string, Client*>::iterator it = members.find(nickname);
    if (it != members.end()) {
        members.erase(it);
    }
    operators.erase(nickname);
}

bool Channel::kickMember(Client* requester, const std::string& targetNick, const std::string& reason) {
    if (!requester) return false;
    const std::string reqNick = requester->getNickname();
    if (!isOperator(reqNick)) {
        requester->sendMessage(":" + std::string(SERVERNAME) + "482 " + reqNick + " " + chan_name + " : You're not channel operator");
        return false;
    }
    if (!isMember(targetNick)) {
        requester->sendMessage(":" + std::string(SERVERNAME) + "441 " + reqNick + " " + targetNick + " " + chan_name + " : They aren't on that channel");
        return false;
    }
    Client* targetClient = members[targetNick];
    
    // Broadcast KICK: <requester> KICK <channel> <target> : <reason>
    std::string kickMsg = ":" + reqNick + " KICK " + chan_name + " " + targetNick + " :" + (reason.empty() ? "Kicked" : reason);
    broadcast(kickMsg, NULL);
    
    // notify the kicked user
    if (targetClient) targetClient->sendMessage(kickMsg);

    removeMember(targetNick);
    return true;
}

void Channel::addOperator(const std::string& nickname) {
    std::map<std::string, Client*>::iterator it = members.find(nickname);
    if (it != members.end())
    {
        operators[nickname] = it->second;
    }
}

void Channel::removeOperator(const std::string& nickname) {
    operators.erase(nickname);
}

/* =========== authorization check ==============*/
bool Channel::canChangeTopic(const std::string& nickname) const {
    if (!topicRestriction) return true;
    return isOperator(nickname);
}

bool Channel::canJoin(Client* client, const std::string& password) {
    if (!client) return false;
    const std::string nick = client->getNickname();
    if (isMember(nick)) {
        client->sendMessage(":" + std::string(SERVERNAME) + "443 " + nick + "  " + chan_name + " :Is already on channel");
        return false;
    }
    if (!chan_passkey.empty() && chan_passkey != password) {
        client->sendMessage(":" + std::string(SERVERNAME) + "475 " + nick + " " + chan_name + " :Cannot join channel (+k)");
        return false;
    }
    if (inviteOnly && !isInvited(nick)) {
        client->sendMessage(":" + std::string(SERVERNAME) + "473 " + nick + " " + chan_name + " :Cannot join channel (+i)");
        return false;
    }
    if (isFull()) {
        client->sendMessage(":" + std::string(SERVERNAME) + "471 " + nick + " " + chan_name + " :Cannot join channel (+l)");
        return false;
    }
    return true;
}

void Channel::broadcast(const std::string& msg, Client* exclude) {
    std::map<std::string, Client*>::iterator it = members.begin();
    for (; it != members.end(); ++it) {
        Client* c = it->second;
        if (exclude && c == exclude) continue;
        if (c) {
            c->sendMessage(msg);
        }
    }
}


