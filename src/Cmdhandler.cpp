/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cmdhandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 16:54:06 by xhuang            #+#    #+#             */
/*   Updated: 2025/10/14 22:40:49 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include "../inc/Parser.hpp" // parseLine

void Server::handlePass(Client* c, const std::vector<std::string>& params) {
    if (!c) return;
    if (c->isRegistered()) { c->sendMessage(":" SERVER_NAME " " ERR_ALREADYREGISTRED " " + c->getNickname() + " :You may not reregister"); return; }
    if (params.empty()) { c->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " * PASS :Not enough parameters"); return; }
    
    if (params[0] != password) {
        c->sendMessage(":" SERVER_NAME " " ERR_PASSWDMISMATCH " * :Password incorrect");
        // 友好地给客户端时间读取，再关闭
        // c->appendInbuff("Closing connection due to incorrect password.\r\n");// or snedMessage?
        ::shutdown(c->getFd(), SHUT_RDWR);
        return;
    }
    c->setPassOk(password);
    c->setRegistered();
    if (c->isRegistered()) { sendWelcome(c); }
}

void Server::handleNick(Client* c, const std::vector<std::string>& params) {
    if (!c) return;
    if (params.empty()) { c->sendMessage(":" SERVER_NAME " " ERR_NONICKNAMEGIVEN " * :No nickname given"); return; }
    
    const std::string nick = params[0];
    for (ClientMap::iterator it = client_lst.begin(); it != client_lst.end(); ++it) {
        Client* other = it->second;
        if (other != c && other->getNickname() == nick) {
            c->sendMessage(":" SERVER_NAME " " ERR_NICKNAMEINUSE " * " + nick + " :Nickname is already in use");
            return;
        }
    }

    const std::string oldnick = c->getNickname();
    c->setNickname(nick);
    if (!oldnick.empty() && oldnick != nick) {
        // for each channel: ch->broadcast(...) if needed
        c->sendMessage(":" + oldnick + " NICK :" + nick);
    }
    c->setRegistered();
    if (c->isRegistered() && oldnick.empty()) {
        sendWelcome(c);
    }
}

void Server::handleUser(Client* c, const std::vector<std::string>& params) {
    if (!c) return;
    if (c->isRegistered()) { c->sendMessage(":" SERVER_NAME " " ERR_ALREADYREGISTRED " " + c->getNickname() + " :You may not reregister"); return; }
    
    if (params.size() < 4) {
        c->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " * USER :Not enough parameters");
        return;
    }
    const std::string username = params[0];
    c->setUsername(username);
    c->setRegistered();
    if (c->isRegistered()) { sendWelcome(c);}
}

//JOIN #tea\r\n
void Server::handleJoin(Client* c, const std::vector<std::string>& params) {
    if (!c) return;
    if (!c->isRegistered()) { c->sendMessage(":" SERVER_NAME " " ERR_NOTREGISTERED " * :You have not registered"); return; }
    if (params.empty()) { c->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " " + c->getNickname() + " JOIN :Not enough parameters"); return; }
    
    // one channel + optional key
    std::string chan = params[0];
    std::string key  = params.size() > 1 ? params[1] : "";
    if (chan.empty() || chan[0] != '#') {
        c->sendMessage(":" SERVER_NAME " " ERR_NOSUCHCHANNEL " " + c->getNickname() + " " + chan + " :No such channel");
        return;
    }
    Channel* ch = createChannel(chan);
    if (!ch->addMember(c, key)) { return; }
    //send names list already in addMember
    c->joinChannel(chan);
}

//PART #tea :gotta run\r\n
void Server::handlePart(Client* c, const std::vector<std::string>& params) {
    if (!c) return;
    if (!c->isRegistered()) { c->sendMessage(":" SERVER_NAME " " ERR_NOTREGISTERED " * :You have not registered"); return; }
    if (params.empty()) { c->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " " + c->getNickname() + " PART :Not enough parameters"); return; }

    std::string chan = params[0];
    std::string reason = params.size() > 1 ? params[1] : "";
    std::map<std::string, Channel*>::iterator it = channel_lst.find(chan);
    if (it == channel_lst.end()) {
        c->sendMessage(":" SERVER_NAME " " ERR_NOSUCHCHANNEL " " + c->getNickname() + " " + chan + " :No such channel");
        return;
    }
    Channel* ch = it->second;
    if (!ch->isMember(c->getNickname())) {
        c->sendMessage(":" SERVER_NAME " " ERR_NOTONCHANNEL " " + c->getNickname() + " " + chan + " :You're not on that channel");
        return;
    }
    // Broadcast PART message to channel members
    ch->broadcastInChan(":" + c->getNickname() + " PART " + chan + " :" + reason, NULL);
    c->sendMessage(":" + c->getNickname() + " PART " + chan + " :" + reason);
    Log::partEvt(c->getNickname(), chan, reason);
    // Remove member from channel
    ch->removeMember(c->getNickname());
    c->leaveChannel(chan);
    // delete empty channel
    if (ch->getMemberCount() == 0) {
        channel_lst.erase(it); 
        delete ch; 
    }
}

//TOPIC #tea :Tea time at 5pm\r\n
void Server::handleTopic(Client* client, const std::vector<std::string>& params) {
    if (!client) return;
    if (!client->isRegistered()) { client->sendMessage(":" SERVER_NAME " " ERR_NOTREGISTERED " * :You have not registered"); return; }
    if (params.size() < 1) { client->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " " + client->getNickname() + " TOPIC :Not enough parameters"); return; }
    std::string chan = params[0];
    ChannelMap::iterator it = channel_lst.find(chan);
    if (it == channel_lst.end()) { client->sendMessage(":" SERVER_NAME " " ERR_NOSUCHCHANNEL " " + client->getNickname() + " " + chan + " :No such channel"); return; }
    
    Channel* c = it->second;
    if (params.size() == 1) {
        c->sendTopic(client);
    } else {
        c->setTopic(params[1], client); 
    }
}

//KICK #tea bob :spamming\r\n
void Server::handleKick(Client* client, const std::vector<std::string>& params) {
    if (!client) return;
    if (!client->isRegistered()) { client->sendMessage(":" SERVER_NAME " " ERR_NOTREGISTERED " * :You have not registered"); return; }
    if (params.size() < 2) { client->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " " + client->getNickname() + " KICK :Not enough parameters"); return; }

    std::string chan = params[0];
    std::string target = params[1];
    std::string reason = params.size() > 2 ? params[2] : "";
    ChannelMap::iterator it = channel_lst.find(chan);
    if (it == channel_lst.end()) { client->sendMessage(":" SERVER_NAME " " ERR_NOSUCHCHANNEL " " + client->getNickname() + " " + chan + " :No such channel"); return; }
    
    Channel* c = it->second;
    if (!c->isOperator(client->getNickname())) { client->sendMessage(":" SERVER_NAME " " ERR_CHANOPRIVSNEEDED " " + client->getNickname() + " " + chan + " :You're not channel operator"); return; }
    if (!c->isMember(target)) { client->sendMessage(":" SERVER_NAME " " ERR_NOTONCHANNEL " " + client->getNickname() + " " + chan + " :You're not on that channel"); return; }
    c->broadcastInChan(":" + client->getNickname() + " KICK " + chan + " " + target, NULL);
    c->kickMember(client, target, reason);
}

       


//todo, unclear IRC message format
void Server::handleMode(Client* client, const std::vector<std::string>& params) {
    if (!client) return;
    if (!client->isRegistered()) { client->sendMessage(":" SERVER_NAME " " ERR_NOTREGISTERED " * :You have not registered"); return; }

    IRCmessage msg; msg.command = "MODE"; msg.params = params;
    std::string chan = params[0];
    char mode = params[1][1];
    // Minimal stub; real mode handling requires parsing and Channel setters
    (void)params; // avoid unused parameter warning under -Werror
    client->sendMessage(":server 324 " + client->getNickname() + " :modes not implemented");
}

//INVITE bob #tea\r\n
void Server::handleInvite(Client* client, const std::vector<std::string>& params) {
    if (!client) return;
    if (!client->isRegistered()) { client->sendMessage(":" SERVER_NAME " " ERR_NOTREGISTERED " * :You have not registered"); return; }
    if (params.size() < 2) { client->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " " + client->getNickname() + " INVITE :Not enough parameters"); return; }

    std::string nick = params[0];
    std::string chan = params[1];
    ChannelMap::iterator it = channel_lst.find(chan);
    if (it == channel_lst.end()) { client->sendMessage(":" SERVER_NAME " " ERR_NOSUCHCHANNEL " " + client->getNickname() + " " + chan + " :No such channel"); return; }
    Channel* c = it->second;
    if (!c->isMember(client->getNickname())) {
        client->sendMessage(":" SERVER_NAME " " ERR_NOTONCHANNEL " " + client->getNickname() + " " + chan + " :You're not on that channel");
        return;
    }
    if (c->getModesString() == "+i" && !c->isOperator(client->getNickname())) {
        client->sendMessage(":" SERVER_NAME " " ERR_CHANOPRIVSNEEDED " " + client->getNickname() + " " + chan + " :You're not channel operator");
        return;
    }
    Client* target = findClientByNick(nick);
    if (!target) { client->sendMessage(":" SERVER_NAME " " ERR_NOSUCHNICK " " + client->getNickname() + " " + nick + " :No such nick"); return; }
    if(!c->inviteUser(nick)) {
        client->sendMessage(":" SERVER_NAME " NOTICE " + client->getNickname() + " :" + target->getNickname() + " is already invited to " + c->getName());
    }
    
    client->sendMessage(":" SERVER_NAME " " RPL_INVITING " " + client->getNickname() + " " + nick + " " + chan);
    target->sendMessage(":" + client->getNickname() + " INVITE " + nick + " :" + chan);
}


void Server::handlePrivmsg(Client* client, const std::vector<std::string>& params) {
    if (!client) return;
    if (!client->isRegistered()) { client->sendMessage(":" SERVER_NAME " " ERR_NOTREGISTERED " * :You have not registered"); return; }
    if (params.size() < 2) { client->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " " + client->getNickname() + " PRIVMSG :Not enough parameters"); return; }

    const std::string target = params[0];
    const std::string text   = params[1];
    const std::string nick   = client->getNickname();
    
    // Channel message
    if (!target.empty() && target[0] == '#') {
        ChannelMap::iterator it = channel_lst.find(target);
        if (it == channel_lst.end()) { client->sendMessage(":" SERVER_NAME " " ERR_NOSUCHCHANNEL " " + nick + " " + target + " :No such channel"); return; }
        Channel* ch = it->second;
        if (!ch->isMember(nick)) { client->sendMessage(":" SERVER_NAME " " ERR_CANNOTSENDTOCHAN " " + nick + " " + target + " :Cannot send to channel"); return; }
        ch->broadcastInChan(":" + nick + " PRIVMSG " + target + " :" + text, client);
        return;

    }
    // Try direct user-to-user first
    Client* dst = findClientByNick(target);
    if (!dst) { client->sendMessage(":" SERVER_NAME " " ERR_NOSUCHNICK " " + nick + " " + target + " :No such nick"); return; }
    dst->sendMessage(":" + nick + " PRIVMSG " + target + " :" + text);
    
    // If no such nick, broadcast to all registered clients (simple roomless chat)
    // for (size_t k = 1; k < pollfds.size(); ++k) {
    //     int other = pollfds[k].fd;
    //     Client* c = clients[other];
    //     if (!c || c == client) continue; // skip sender here
    //     c->sendMessage(":" + client->getNickname() + " PRIVMSG * :" + msg);
    //     pollfds[k].events |= POLLOUT;
    // }
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



void Server::handleCmd(Client* c, const std::string& line) {
    if (!c || line.empty()) return;
    // Parse the command and execute appropriate actions
    IRCmessage msg = parseLine(line);
    if (msg.command.empty()) return;
    // log
    std::cout << "fd=" << c->getFd() << " CMD: " << msg.command << " ("
              << (msg.params.empty() ? "" : msg.params[0]) << " ...)\n";

    if (msg.command == "PASS")  handlePass(c, msg.params);//PASS <password>
    else if (msg.command == "NICK") handleNick(c, msg.params);
    else if (msg.command == "USER") handleUser(c, msg.params);
    else if (msg.command == "JOIN")  handleJoin(c, msg.params);
    else if (msg.command == "PART")    handlePart(c, msg.params);
    else if (msg.command == "PRIVMSG")    
        handlePrivmsg(c, msg.params);
    else if (msg.command == "QUIT")//mode invite and kick
        handleQuit(c, msg.params);
    else if (msg.command == "MODE")
    {
    
    }    
    else if (msg.command == "KICK")  handleKick(c, msg.params);
    else if (msg.command == "INVITE")  handleInvite(c, msg.params);//todo
    else if (msg.command == "TOPIC")   handleTopic(c, msg.params);
    else
        c->sendMessage(":" SERVER_NAME " " ERR_UNKNOWNCOMMAND " " + c->getNickname() + " " + msg.command + " :Unknown command");
} 