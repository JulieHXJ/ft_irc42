/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmonika <mmonika@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 23:56:57 by mmonika           #+#    #+#             */
/*   Updated: 2025/11/30 14:54:20 by mmonika          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"
#include "../inc/Global.hpp"
#include "../inc/Log.hpp"
#include <vector>

Channel::Channel(const std::string& name) : name(name), topic(), passKey(), inviteOnly(false), topicRestriction(false), maxUserLimit(0) {}
Channel::~Channel() {
    members.clear();
    operators.clear();
    invitedUsers.clear();
}
std::string Channel::getName() const { return name; }
std::string Channel::getTopic() const { return topic; }
size_t Channel::getUserLimit() const { return maxUserLimit; }
int Channel::getMemberCount() const { return static_cast<int>(members.size()); }
bool Channel::isFull() const { return (maxUserLimit > 0 && members.size() >= maxUserLimit); }

bool Channel::addMember(Client* client, const std::string& password) {
    if (!client) return false;
    if (!canJoin(client, password)) return false;
    members[client->getNickname()] = client;
    invitedUsers.erase(client->getNickname());
    if (members.size() == 1) operators.insert(client->getNickname());
    broadcast(":" + client->getNickname() + " JOIN " + name, nullptr);
    if (topic.empty())
        client->sendMessage(std::string(":") + SERVER_NAME + " " + RPL_NOTOPIC + " " + client->getNickname() + " " + name + " :No topic is set");
    else
        client->sendMessage(std::string(":") + SERVER_NAME + " " + RPL_TOPIC + " " + client->getNickname() + " " + name + " :" + topic);
    sendNamesList(client);
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

bool Channel::addOperator(const std::string& nickname) {
    if (!isMember(nickname)) {
        Log::warn("[MODE] cannot +o: " + nickname + " not in " + name);
        return false;
    }
    if (!operators.insert(nickname).second) {
        Log::warn("[MODE] " + nickname + " is already operator in " + name);
        return false;
    }
    Log::info("[MODE] +o " + name + ": " + nickname + " is now an operator");
    return true;
}

bool Channel::removeOperator(const std::string& nickname) {
    if (!isOperator(nickname)) {
        Log::warn("[MODE] cannot -o: " + nickname + " is not an operator in " + name);
        return false;
    }
    operators.erase(nickname);
    Log::info("[MODE] -o " + name + ": " + nickname + " is no longer an operator");
    return true;
}

bool Channel::inviteUser(const std::string& nickname) {
    if (nickname.empty()) return false;
    if (isInvited(nickname)) {
        Log::warn("[INVITE] " + nickname + " is already invited to " + name);
        return false;
    }
    invitedUsers.insert(nickname);
    return true;
}

bool Channel::isInvited(const std::string& nickname) const {
    return invitedUsers.find(nickname) != invitedUsers.end();
}

void Channel::broadcast(const std::string &msg, Client* exclude) {
    for (auto& entry : members) {
        Client* c = entry.second;
        if (!c || c == exclude) continue;
        if (!c->sendMessage(msg)) {
            Log::warn("[BROADCAST] failed to send to " + entry.first);
        }
    }
}

void Channel::setMode(char mode, bool set, const std::string& param) {
    switch (mode) {
        case 'i': inviteOnly = set; break;            
        case 't': topicRestriction = set; break;
        case 'k':
            if (set) {
                if (param.empty()) return;
                passKey = param ;
            } else passKey.clear();
            break;
        case 'o':
            if (param.empty() || !isMember(param)) return;
            if (set) addOperator(param); else removeOperator(param);
            break;
        case 'l':
            if (set) {
                if (param.empty()) return;
                size_t limit = 0;
                for (char c : param) { 
                    if (!std::isdigit(static_cast<unsigned char>(c))) return;
                    limit = limit * 10 + (c - '0');
                    if (limit > 10000) break;
                }
                if (limit == 0) return;
                maxUserLimit = limit;
            } else { maxUserLimit = 0; }
            break;
        default: break;
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
    if (!client) return;
    if (!canChangeTopic(client->getNickname())) {
        client->sendMessage(std::string(":") + SERVER_NAME + " " + ERR_CHANOPRIVSNEEDED + " " + client->getNickname() + " " + name + " :You're not channel operator"); //need to check again
        return;
    }
    topic = newTopic;
    broadcast(":" + client->getNickname() + " TOPIC " + name + " :" + topic, nullptr);
    Log::topicEvt(client->getNickname(), name, topic);
}

bool Channel::canChangeTopic(const std::string& nickname) const {
    return (!topicRestriction) || isOperator(nickname);
}

bool Channel::kickMember(Client* client, const std::string& targetNickname, const std::string& reason) {
    if (!client) return false;
    if (!isOperator(client->getNickname())) {
        client->sendMessage(std::string(":") + SERVER_NAME + " " + ERR_CHANOPRIVSNEEDED + " " + client->getNickname() + " " + name + " :You're not channel operator");
        return false;
    }
    if (!isMember(targetNickname)) {
        client->sendMessage(std::string(":") + SERVER_NAME + " " + ERR_USERNOTINCHANNEL + " " + client->getNickname() + " " + targetNickname + " " + name + " :They aren't on that channel");
        return false;
    }
    Client* targetClient = members[targetNickname];
    std::string kickMsg = ":" + client->getNickname() + " KICK " + name + " " + targetNickname;
    if (!reason.empty()) kickMsg += " :" + reason;
    Log::kickEvt(client->getNickname(), name, targetNickname, reason);
    removeMember(targetNickname);
    broadcast(kickMsg, nullptr);
    if (targetClient) targetClient->sendMessage(kickMsg);
    return true;
}

bool Channel::canJoin(Client* client, const std::string& password) {
    if (!client) return false;
    if (isMember(client->getNickname())) {
        client->sendMessage(std::string(":") + SERVER_NAME + " " + ERR_USERONCHANNEL + " " + client->getNickname() + "  " + name + " :Is already on channel");
        return false;
    }
    if (!passKey.empty() && passKey != password) {
        client->sendMessage(std::string(":") + SERVER_NAME + " " + ERR_BADCHANNELKEY + " " + client->getNickname() + " " + name + " :Cannot join channel (+k)");
        return false;
    }
    if (inviteOnly && !isInvited(client->getNickname())) {
        client->sendMessage(std::string(":") + SERVER_NAME + " " + ERR_INVITEONLYCHAN + " " + client->getNickname() + " " + name + " :Cannot join channel (+i)");
        return false;
    }
    if (isFull()) {
        client->sendMessage(std::string(":") + SERVER_NAME + " " + ERR_CHANNELISFULL + " " + client->getNickname() + " " + name + " :Cannot join channel (+l)");
        return false;
    }
    return true;
}

void Channel::sendNamesList(Client* client) {
    if (!client) return;
    std::vector<std::string> names;
    names.reserve(members.size());
    for (const auto& entry : members) {
        const std::string& nick = entry.first;
        names.push_back(isOperator(nick) ? "@" + nick : nick);
    }
    const std::string header = std::string(":") + SERVER_NAME + " " + RPL_NAMREPLY + " " + client->getNickname() + " = " + name + " :";
    std::string line;
    for (size_t i = 0; i < names.size(); ++i) {
        const std::string& token = names[i];
        const std::string candidate = line.empty() ? token : (line + " " + token);
        if (header.size() + candidate.size() > 480) {
            client->sendMessage(header + line);
            line = token;
        } else {
            line = candidate;
        }
    }
    client->sendMessage(line.empty() ? header : header + line);
    client->sendMessage(std::string(":") + SERVER_NAME + " " + RPL_ENDOFNAMES + " " + client->getNickname() + " " + name + " :End of NAMES list");
}

void Channel::sendTopic(Client* client) {
    if (!client) return;
    if (!isMember(client->getNickname())) {
        client->sendMessage(std::string(":") + SERVER_NAME + " " + ERR_NOTONCHANNEL + " " + client->getNickname() + " " + name + " :You're not on that channel");
        return;
    }
    if (!topic.empty())
        client->sendMessage(std::string(":") + SERVER_NAME + " " + RPL_TOPIC + " " + client->getNickname() + " " + name + " :" + topic);
    else
        client->sendMessage(std::string(":") + SERVER_NAME + " " + RPL_NOTOPIC + " " + client->getNickname() + " " + name + " :No topic is set");
}
