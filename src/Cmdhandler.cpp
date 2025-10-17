/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cmdhandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 16:54:06 by xhuang            #+#    #+#             */
/*   Updated: 2025/10/17 17:23:45 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include "../inc/Parser.hpp" // parseLine

void Server::handlePass(Client* c, const std::vector<std::string>& params) {
    if (!c) return;
    if (c->isRegistered()) {
        c->sendMessage(":" SERVER_NAME " " ERR_ALREADYREGISTRED " " + c->getNickname() + " :You may not reregister");
        return;
    }
    if (params.empty()) { c->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " * PASS :Not enough parameters"); return; }
    
    if (!password.empty() && params[0] != password) {
        c->sendMessage(":" SERVER_NAME " " ERR_PASSWDMISMATCH " * :Password incorrect");
        c->markForClose();// shutdown the client (optional but usual)
        return;
    }
    Log::info ("PASS ok fd=" + std::to_string(c->getFd()));
    c->setPassOk(password);
    c->setRegistered();
    if (c->isRegistered()) {
        sendWelcome(c);
        Log::info("Client fd=" + std::to_string(c->getFd()) +
                  " (" + c->getNickname() + ") has completed registration.");
    }
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
    if (c->isRegistered()) {
        sendWelcome(c);
        Log::info("Client fd=" + std::to_string(c->getFd()) +
                  " (" + c->getNickname() + ") has completed registration.");
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
    if (c->isRegistered()) {
        sendWelcome(c);
        Log::info("Client fd=" + std::to_string(c->getFd()) +
                  " (" + c->getNickname() + ") has completed registration.");
    }
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
    if (!ch->addMember(c, key)) { 
        Log::warn(c->getNickname() + " failed to join " + chan);
        return; 
    }
    //send names list already in addMember
    c->joinChannel(chan);
    Log::joinEvt(c->getNickname(), chan);
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
    Log::partEvt(c->getNickname(), chan, reason);
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
    Log::topicEvt(client->getNickname(), chan, params.size() > 1 ? params[1] : "");
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
    Log::kickEvt(client->getNickname(), chan, target, reason);
}

//INVITE bob #tea\r\n
void Server::handleInvite(Client* client, const std::vector<std::string>& params) {
    if (!client) return;
    if (!client->isRegistered()) { client->sendMessage(":" SERVER_NAME " " ERR_NOTREGISTERED " * :You have not registered"); return; }
    if (params.size() < 2) { client->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " " + client->getNickname() + " INVITE :Not enough parameters"); return; }

    std::string nick = params[0];
    std::string chan = params[1];

    //find the channel
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

    //find client by nick
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
    
}

// QUIT :<message>
void Server::handleQuit(Client* client, const std::vector<std::string>& params){
    if (!client) return;
    const std::string nick = client->getNickname();
    const std::string reason = params.empty() ? "" : params[0];
    
    ChannelMap::iterator it = channel_lst.begin();
    while (it != channel_lst.end()) {
        Channel* ch = it->second;
        ChannelMap::iterator next = it;
        ++next;
        if (ch && ch->isMember(nick)) {
            ch->broadcastInChan(":" + nick + " QUIT :" + reason, NULL);
            ch->removeMember(nick);
            client->leaveChannel(ch->getName());
            // possibly delete empty channel
            if (ch->getMemberCount() == 0) {
                channel_lst.erase(it);
                delete ch;
            }
        }
        it = next;
    }
    // cleanupEmptyChannels();
    Log::info("Client fd=" + std::to_string(client->getFd()) + " (" + nick + ") QUIT: " + reason);
    client->markForClose();
}


// MODE #chan [modes [params...]]
// MODE <nick>
void Server::handleMode(Client* client, const std::vector<std::string>& params) {
    if (!client) return;
    if (!client->isRegistered()) { client->sendMessage(":" SERVER_NAME " " ERR_NOTREGISTERED " * :You have not registered"); return; }
    if (params.empty()) { client->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " " + client->getNickname() + " MODE :Not enough parameters"); return; }
    
    const std::string target = params[0];
    
    // ----- MODE #chan -----
    if (!target.empty() && target[0] == '#') {
        ChannelMap::iterator it = channel_lst.find(target);
        if (it == channel_lst.end()) {
            client->sendMessage(":" SERVER_NAME " " ERR_NOSUCHCHANNEL " " + client->getNickname() + " " + target + " :No such channel");
            return;
        }
        Channel* ch = it->second;
        
        if (params.size() == 1) {
            std::string modes = ch->getModesString();
            std::string tail;                        

            if (modes.find('k') != std::string::npos) {
                Log::info("passkey set for channel " + target);
                tail += " *";
            }
            if (modes.find('l') != std::string::npos) {
                size_t lim = ch->getUserLimit();
                if (lim > 0) tail += " " + std::to_string(lim);
            }

            client->sendMessage(":" SERVER_NAME " " RPL_CHANNELMODEIS " " + client->getNickname() + " " + target + " " + modes + tail);
            return;
        }

        // validate operator privilege
        if (!ch->isOperator(client->getNickname())) {
            client->sendMessage(":" SERVER_NAME " " ERR_CHANOPRIVSNEEDED " " + client->getNickname() + " " + target + " :You're not channel operator");
            return;
        }

        // params: [0]=#chan, [1]=modes, [2...]=args
        const std::string modes = params[1];
        if (modes.empty() || (modes[0] != '+' && modes[0] != '-')) {
            client->sendMessage(":" SERVER_NAME " " ERR_UNKNOWNMODE " " + client->getNickname() + " " + target + " :Unknown mode flag");
            return;
        }

        bool set = (modes[0] == '+');
        size_t argIdx = 2;

        for (size_t i = 1; i < modes.size(); ++i) {
            char m = modes[i];
            std::string param; 

            switch (m) {
                case 'i': // invite-only
                case 't': // topic-restriction
                    ch->setMode(m, set, "");
                    break;

                case 'k': // passkey
                    if (set) {
                        if (argIdx >= params.size()) {
                            client->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " " + client->getNickname() + " MODE :Not enough parameters");
                            return;
                        }
                        param = params[argIdx++];
                        ch->setMode('k', true, param);
                    } else {
                        ch->setMode('k', false, "");
                    }
                    break;

                case 'l': // user limit
                    if (set) {
                        if (argIdx >= params.size()) {
                            client->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " " + client->getNickname() + " MODE :Not enough parameters");
                            return;
                        }
                        param = params[argIdx++];
                        ch->setMode('l', true, param);
                    } else {
                        ch->setMode('l', false, "");
                    }
                    break;

                case 'o': // operator add/remove
                    if (argIdx >= params.size()) {
                        client->sendMessage(":" SERVER_NAME " " ERR_NEEDMOREPARAMS " " + client->getNickname() + " MODE :Not enough parameters");
                        return;
                    }
                    param = params[argIdx++];
                    ch->setMode('o', set, param);
                    break;

                default:
                    client->sendMessage(":" SERVER_NAME " " ERR_UNKNOWNMODE " " + client->getNickname() + " " + std::string(1, m) + " :is unknown mode char to me");
                    break;
            }
        }

        // broadcast
        std::string broadcast = ":" + client->getNickname() + " MODE " + target + " " + modes;
        for (size_t i = 2; i < argIdx; ++i) {
            broadcast += " " + params[i];
        }
        ch->broadcastInChan(broadcast, NULL);
        client->sendMessage(broadcast);
        return;
    }

    // ----- MODE <nick> -----
    if (!target.empty() && target[0] != '#' && params.size() == 1) {
        Client* targetC = findClientByNick(target);
        if (!targetC) {
            client->sendMessage(":" SERVER_NAME " 401 " + client->getNickname() + " " + target + " :No such nick");
            return;
        }
        std::string chans;
        const std::vector<std::string>& joined = targetC->getChannels();
        for (std::vector<std::string>::const_iterator it = joined.begin(); it != joined.end(); ++it) {
            chans += *it + " ";
        }

        client->sendMessage(":" SERVER_NAME " 319 " + client->getNickname() + " " + target + " :" + chans);
        return;
    }

    client->sendMessage(":" SERVER_NAME " " ERR_UMODEUNKNOWNFLAG " " + client->getNickname() + " :User mode handling not implemented");
}


void Server::handleCmd(Client* c, const std::string& line) {
    if (!c || line.empty()) return;
    IRCmessage msg = parseLine(line);
    if (msg.command.empty()) return;

    if (msg.command == "PASS")  handlePass(c, msg.params);//PASS <password>
    else if (msg.command == "NICK") handleNick(c, msg.params);
    else if (msg.command == "USER") handleUser(c, msg.params);
    else if (msg.command == "JOIN")  handleJoin(c, msg.params);
    else if (msg.command == "PART")    handlePart(c, msg.params);
    else if (msg.command == "PRIVMSG") handlePrivmsg(c, msg.params);
    else if (msg.command == "QUIT")  handleQuit(c, msg.params);
    else if (msg.command == "MODE")   handleMode(c, msg.params);  
    else if (msg.command == "KICK")  handleKick(c, msg.params);
    else if (msg.command == "INVITE")  handleInvite(c, msg.params);
    else if (msg.command == "TOPIC")   handleTopic(c, msg.params);
    else if (msg.command == "LIST")   handleList(c, msg.params);
    else if (msg.command == "NAMES")   handleNames(c, msg.params);
    else if (msg.command == "INFO")   handleInfo(c);
    else
        c->sendMessage(":" SERVER_NAME " " ERR_UNKNOWNCOMMAND " " + c->getNickname() + " " + msg.command + " :Unknown command");
} 


//LIST #keyword (for searching channles with keyword)
void Server::handleList(Client* c, const std::vector<std::string>& params) {
    if (!c || !c->isRegistered()) return;
    std::string pattern;
    if (!params.empty()) pattern = params[0]; 
    for (ChannelMap::iterator it = channel_lst.begin(); it != channel_lst.end(); ++it) {
        Channel* ch = it->second;
        if (!pattern.empty() && ch->getName().find(pattern) == std::string::npos)
            continue;
        c->sendMessage(":" SERVER_NAME " 322 " + c->getNickname() + " " +
                       ch->getName() + " " + std::to_string(ch->getMemberCount()) +
                       " :" + ch->getTopic());
    }
    c->sendMessage(":" SERVER_NAME " 323 " + c->getNickname() + " :End of LIST");
}

// NAMES（#channel）
void Server::handleNames(Client* c, const std::vector<std::string>& params) {
	if (!c || !c->isRegistered()) return;
	if (params.empty()) {
		// List names for all channels
		for (ChannelMap::iterator it = channel_lst.begin(); it != channel_lst.end(); ++it) {
			Channel* ch = it->second;
			ch->sendNamesList(c);
		}
	} else {
		// List names for specified channels
		for (size_t i = 0; i < params.size(); ++i) {
			const std::string& chanName = params[i];
			ChannelMap::iterator it = channel_lst.find(chanName);
			if (it != channel_lst.end()) {
				Channel* ch = it->second;
				ch->sendNamesList(c);
			} else {
				// No such channel, send empty NAMES reply
				c->sendMessage(":" SERVER_NAME " 353 " + c->getNickname() + " = " + chanName + " :");
				c->sendMessage(":" SERVER_NAME " 366 " + c->getNickname() + " " + chanName + " :End of NAMES list");
			}
		}
	}
}

// ADMIN
void Server::handleInfo(Client* c) {
    if (!c) return;
    c->sendMessage(":" SERVER_NAME " 256 " + c->getNickname() + " :Administrative info");
    c->sendMessage(":" SERVER_NAME " 257 " + c->getNickname() + " :Admin: xhuang, mmonika & gahmed (Server Owner)");
    c->sendMessage(":" SERVER_NAME " 259 " + c->getNickname() + " :Location: Heilbronn, Germany");
}