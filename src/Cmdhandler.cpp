/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cmdhandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 16:54:06 by xhuang            #+#    #+#             */
/*   Updated: 2025/10/09 23:48:32 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include "../inc/Parser.hpp" // parseLine

void Server::handleLine(int fd, const std::string& rawLine) {
    //if line is empty , ignore
    if (rawLine.empty()) return;
    // Parse the command and execute appropriate actions
    Message msg = parseLine(rawLine);
    if (msg.command.empty()) return;
    std::cout << "Received command from fd=" << fd << ": " << rawLine << "\n";
    

    if (msg.command == "NICK")
        handleNick(cl, msg);
    else if (msg.command == "PASS")
    {
        handlePASS(fd, msg.params[0]);//PASS <password>
    }
    
    else if (msg.command == "USER")
        handleUser(cl, msg);
    else if (msg.command == "JOIN")
        handleJoin(cl, msg);
    else if (msg.command == "PART")
        handlePart(cl, msg);
    else if (msg.command == "PRIVMSG")
        handlePrivmsg(cl, msg);
    else if (msg.command == "QUIT")
        handleQuit(cl, msg);
    else
        cl.sendMessage(":server 421 " + c.getNickname() + " " + msg.command + " :Unknown command");
}

void Server::handlePASS(int fd, const std::string& pass) {
    std::map<int, Client>::iterator it = client_lst.find(fd);
    if (it == client_lst.end()) return;
    Client& c = it->second;

    if (c.isRegistered()) {
        pushToClient(fd, numReply("462", c.getNickname(), ":You may not reregister"));
        return;
    }
    if (pass.empty()) {
        pushToClient(fd, numReply("461", c.getNickname(), "PASS :Not enough parameters"));
        return;
    }
    if (pass != password) {
        // 464 ERR_PASSWDMISMATCH
        pushToClient(fd, numReply("464", c.getNickname(), ":Password incorrect"));
        // 友好地给客户端时间读取，再关闭
        c.getOutput() += CRLF;
        // 直接清理
        // 注意：不要在这里立刻 erase poll 索引；交给主循环的 handleWritable 之后或设置一个标志。
        // 简化起见，直接关闭：
        ::shutdown(fd, SHUT_RDWR);
        return;
    }
    c.setPassOk(true);
    maybeRegister(c);
}













void Server::handleJoin(Client* c, const std::string& params) {
    if (!c) return;
    // very simple parse: one channel + optional key
    std::string chanName, key;
    // split params by space into chanName and key...
    // (write your own tiny parser; 42-style avoids STL split)

    Channel* ch = newChannel(chanName);
    if (!ch->addMember(c, key)) {
        // addMember already sent numerics on failure
        return;
    }

    // On success, `Channel::addMember` already:
    // - broadcasted JOIN
    // - sent topic (332/331) sendTopic
    // - sent NAMES (353/366) via sendNamesList_
}

//KICK <#chan> <nick> [:reason]
void Server::handleKick(Client* c, const std::string& chanName, const std::string& target, const std::string& reason) {
    if (!c) return;
    std::map<std::string, Channel*>::iterator it = channel_lst.find(chanName);
    if (it == _channels.end()) {
        c->sendMessage(":" SERVER_NAME " 403 " + c->getNickname() + " " + chanName + " :No such channel");
        return;
    }
    it->second->kickMember(c, target, reason);
}

void Server::handleInvite(Client* c, const std::string& chanName, const std::string& targetNick){
    if (!c) return;
    
}

void Server::handleTopic(Client* c, const std::string& chanName, const std::string& newTopic){
    if (!c) return;
    
}



// QUIT :<message>
void Server::handleQuit(Client* c, const std::string& msg){
    if (!c) return;
    const std::string nick = c->getNickname();
    std::map<std::string, Channel*>::iterator it = channel_lst.begin();

    while (it != channel_lst.end()) {
        Channel* ch = it->second;
        if (ch->isMember(nick)) {
            ch->broadcastInChan(":" + nick + " QUIT :" + (msg.empty() ? "Quit" : msg), 0);
            ch->removeMember(nick);
            // possibly delete empty channel
            if (ch->getMemberCount() == 0) {
                cleanupEmptyChannels();
                continue;
            }
        }
        ++it;
    }
}
void Server::cleanupEmptyChannels(){
    std::map<std::string, Channel*>::iterator it = channel_lst.begin();
    while (it != channel_lst.end()) {
        Channel* ch = it->second;
        if (ch->getMemberCount() == 0) {
            Channel* toDelete = ch;
            std::map<std::string, Channel*>::iterator eraseIt = it++;
            channel_lst.erase(eraseIt);
            delete toDelete;
        } else {
            ++it;
        }
    }
}



