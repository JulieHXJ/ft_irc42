/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 18:03:47 by xhuang            #+#    #+#             */
/*   Updated: 2025/10/09 22:52:09 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Channel.hpp"
#include "../inc/Client.hpp" //provide getNickname() and sendMessage()

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
        setter->sendMessage(":" SERVER_NAME " " ERR_CHANOPRIVSNEEDED " " + nick + " " + chan_name + " :You're not channel operator");
        return;
    }
    chan_topic = newTopic;

    // Broadcast TOPIC :<nick> TOPIC <channel> : <topic>
    broadcastInChan(":" + nick + "  TOPIC " + chan_name + " : " + chan_topic, NULL);
    Log::topicEvt(nick, chan_name, newTopic);
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
    if (members.size() == 1u) { operators[nick] = client; }//set first member as operator

    // Broadcast JOIN: <nick> JOIN <channel>
    broadcastInChan(":" + nick + " JOIN " + chan_name, NULL);
    Log::joinEvt(nick, chan_name);

    // Send topic and names list to the new member
    if (chan_topic.empty()) {
        client->sendMessage(":" SERVER_NAME " " RPL_NOTOPIC " " + nick + " " + chan_name + " :No topic is set");
    } else {
        client->sendMessage(":" SERVER_NAME " " RPL_TOPIC " " + nick + " " + chan_name + " :" + chan_topic);
    }
    // Optionally send names list (RPLNAMREPLY/366) later

    // Send names list to the new member (why?)
    sendNamesList(client);//todo
    
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
        requester->sendMessage(":" SERVER_NAME " " ERR_CHANOPRIVSNEEDED " " + reqNick + " " + chan_name + " : You're not channel operator");
        return false;
    }
    if (!isMember(targetNick)) {
        requester->sendMessage(":" SERVER_NAME " " ERR_USERNOTINCHANNEL " " + reqNick + " " + targetNick + " " + chan_name + " : They aren't on that channel");
        return false;
    }
    Client* targetClient = members[targetNick];
    
    // Broadcast KICK: <requester> KICK <channel> <target> : <reason>
    std::string kickMsg = ":" + reqNick + " KICK " + chan_name + " " + targetNick + " :" + (reason.empty() ? "Kicked" : reason);
    broadcastInChan(kickMsg, NULL);
    Log::kickEvt(reqNick, chan_name, targetNick, reason);
    
    // notify the kicked user
    if (targetClient) targetClient->sendMessage(kickMsg);

    removeMember(targetNick);
    return true;
}

bool Channel::addOperator(const std::string& nickname) {
    std::map<std::string, Client*>::iterator it = members.find(nickname);
    if (it == members.end()) {
        Log::warn("[MODE] cannot +o: " + nickname + " is not in " + chan_name);
        return false;
    }
    if (operators.find(nickname) != operators.end())
    {
        Log::warn("[MODE] " + nickname + " is already an operator in " + chan_name);
        return false;
    }
    
    operators[nickname] = it->second;
    Log::info("[MODE] +o" + chan_name + ": " + nickname + " is now an operator");
    return true;
}

bool Channel::removeOperator(const std::string& nickname) {
    std::map<std::string, Client*>::iterator it = operators.find(nickname);
    if (it == operators.end()) {
        Log::warn("[MODE] cannot -o: " + nickname + " is not an operator in " + chan_name);
        return false;
    }
    operators.erase(nickname);
    Log::info("[MODE] -o" + chan_name + ": " + nickname + " is no longer an operator");
    return true;
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
        client->sendMessage(":" SERVER_NAME " " ERR_USERONCHANNEL " " + nick + "  " + chan_name + " :Is already on channel");
        return false;
    }
    if (!chan_passkey.empty() && chan_passkey != password) {
        client->sendMessage(":" SERVER_NAME " " ERR_BADCHANNELKEY " " + nick + " " + chan_name + " :Cannot join channel (+k)");
        return false;
    }
    if (inviteOnly && !isInvited(nick)) {
        client->sendMessage(":" SERVER_NAME " " ERR_INVITEONLYCHAN " " + nick + " " + chan_name + " :Cannot join channel (+i)");
        return false;
    }
    if (isFull()) {
        client->sendMessage(":" SERVER_NAME " " ERR_CHANNELISFULL " " + nick + " " + chan_name + " :Cannot join channel (+l)");
        return false;
    }
    return true;
}

void Channel::broadcastInChan(const std::string& msg, Client* exclude) {
    std::map<std::string, Client*>::iterator it = members.begin();
    for (; it != members.end(); ++it) {
        Client* c = it->second;
        if (exclude && c == exclude) continue;
        if (c) {
            c->sendMessage(msg);
        }
    }
}

void Channel::sendNamesList(Client* to){
    if (!to) return;
    std::string names;
    std::map<std::string, Client*>::iterator it = members.begin();
    for (; it != members.end(); ++it) {
        const std::string& nick = it->first;
        if (isOperator(nick)) {
            names += "@" + nick + " ";
        } else {
            names += nick + " ";
        }
    }
    // 10 names a line
    for (size_t i = 0; i < names.size(); i += 10) {
        std::string chunk;
        for (size_t j = i; j < names.size() && j < i + 10; ++j) {
            if (!chunk.empty()) {
                chunk += " ";
            }
            chunk += names[j];
        }
        // 353: "<nick> = <#chan> :name1 name2 ..."
        to->sendMessage(":" SERVER_NAME " " RPL_NAMREPLY " " + to->getNickname()
                        + " = " + chan_name + " :" + chunk);
    }

    to->sendMessage(":" SERVER_NAME " " RPL_ENDOFNAMES " " + to->getNickname() + " " + chan_name + " :End of NAMES list");
}

void Channel::sendTopic(Client* to){
    
}
