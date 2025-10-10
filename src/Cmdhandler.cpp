/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cmdhandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 16:54:06 by xhuang            #+#    #+#             */
/*   Updated: 2025/10/10 18:46:58 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include "../inc/Parser.hpp" // parseLine

void Server::handlePASS(Client* c, const std::vector<std::string>& params) {
    if (params.empty()) {
        c->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " PASS :Not enough parameters");
        return;
    }
    if (params[0] != password) {
        c->sendMessage(":" SERVER_NAME " " ERR_PASSWDMISMATCH " " + c.getNickname() + " :Password incorrect");

        // 友好地给客户端时间读取，再关闭
        c->appendInbuff("Closing connection due to incorrect password.\r\n");// or snedMessage?
        // 直接清理
        // 注意：不要在这里立刻 erase poll 索引；交给主循环的 handleWritable 之后或设置一个标志。
        // 简化起见，直接关闭：
        ::shutdown(c->getFd(), SHUT_RDWR);
        return;
    }
    c->setPassOk(password);
}

void Server::handleNICK(Client* c, const std::vector<std::string>& params) {
    if (params.empty()) {
        c->sendMessage(":server 431 * :No nickname given");
        return;
    }
    std::string nick = params[0];
    
    for (std::map<int, Client*>::iterator it = client_lst.begin(); it != client_lst.end(); ++it) {
        if (it->second != c && it->second->getNickname() == nick) {
            client->sendMessage(":server 433 * " + nick + " :Nickname is already in use");
            return;
        }
    }
    
    std::string oldnick = c->getNickname();
    client->setNickname(nick);
    
    if (!oldnick.empty()) {
        client->sendMessage(":" + oldnick + " NICK :" + nick);
    }
    
    if (client->isRegistered() && oldnick.empty()) {
        sendWelcome(client);
    }
}



void Server::handleUSER(Client* c, const std::vector<std::string>& params) {
    if (params.size() < 4) {
        client->sendMessage(":server 461 * USER :Not enough parameters");
        return;
    }
    
    std::string username = params[0];
    std::string realname = params[3];
    
    client->setUsername(username, realname);
    
    if (client->isRegistered()) {
        sendWelcome(client);
    }
}





void Server::handleCmd(Client* c, const std::string& Line) {
    //if line is empty , ignore
    if (Line.empty()) return;
    // Parse the command and execute appropriate actions
    IRCmessage msg = parseLine(rawLine);
    if (msg.command.empty()) return;
    std::cout << "Received command from fd=" << fd << ": " << rawLine << "\n";
    

    if (msg.command == "PASS")
        handlePASS(c, msg.params);//PASS <password>
    else if (msg.command == "NICK")    
        handleNICK(c, msg.params);
    else if (msg.command == "USER")
        handleUSER(c, msg.params);
    else if (msg.command == "JOIN")    
        handleJoin(c, msg);
    else if (msg.command == "PART")    
        handlePart(cl, msg);
    else if (msg.command == "PRIVMSG")    
        handlePrivmsg(cl, msg);
    else if (msg.command == "QUIT")//mode invite and kick    
    
        handleQuit(cl, msg);
    else if (msg.command == "MODE")    
    {
        /* code */
    }    
    else if (msg.command == "KICK")    
        handleKick(cl, msg);
    else if (msg.command == "INVITE")    
        handleInvite(cl, msg);
    else if (msg.command == "TOPIC")    
        handleTopic(cl, msg);
    
    else    
        c.sendMessage(":server 421 " + c.getNickname() + " " + msg.command + " :Unknown command");
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



