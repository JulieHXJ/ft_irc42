/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server_cmd.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 16:54:06 by xhuang            #+#    #+#             */
/*   Updated: 2025/10/08 18:39:48 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"

//all need removeMember in Channel class
void Server::handlePart(Client* client, const std::string& channelName);
void Server::handleQuit(Client* client);
void Server::cleanupEmptyChannels();






void Server::parseCmd(const std::string& line, std::string& cmd, std::set<std::string>& args, std::string& trail){
    
}

void Server::handleCmd(int fd, const std::string& line) {
    std::string cmd, trailing;
    //if line is empty or lowcaser case, ignore

    // Parse the command and execute appropriate actions
    parseCmd(line, cmd, trailing);
    if (cmd.empty()) return;
    
    std::cout << "Received command from fd=" << fd << ": " << line << "\n";

    //better to use switch cases for each command
    if (CMD == "NICK") {
        std::string nick; iss >> nick;
        clients_[fd].nick = nick;
        return;
    }
    if (CMD == "USER") {
        std::string user; iss >> user;
        clients_[fd].user = user;
        return;
    }
    if (CMD == "JOIN") {
        std::string chname; iss >> chname;
        if (!chname.empty()) joinChannel(fd, chname);
        return;
    }
    if (CMD == "PART") {
        std::string chname; iss >> chname;
        if (!chname.empty()) partChannel(fd, chname);
        return;
    }
    if (CMD == "PRIVMSG") {
        std::string target; iss >> target;
        std::string msg;
        std::getline(iss, msg);
        if (!msg.empty() && msg[0]==' ') msg.erase(0,1);
        if (!msg.empty() && msg[0]==':') msg.erase(0,1);

        if (!target.empty() && !msg.empty()){
            if (!target.empty() && target[0] == '#') {
                // channel message
                std::map<std::string, Channel>::iterator it = channels_.find(target);
                if (it != channels_.end()){
                    std::string lineOut = ":" + (clients_[fd].nick.empty() ? "anon" : clients_[fd].nick)
                                        + " PRIVMSG " + target + " :" + msg + "\r\n";
                    pushToChannel(it->second, lineOut, fd);
                }
            } else {
                // direct to a user (very naive: by nick scan)
                int toFd = -1;
                for (std::map<int, Client>::iterator itc = clients_.begin(); itc != clients_.end(); ++itc){
                    if (itc->second.nick == target){ toFd = itc->first; break; }
                }
                if (toFd != -1){
                    std::string dm = ":" + (clients_[fd].nick.empty() ? "anon" : clients_[fd].nick)
                                   + " PRIVMSG " + target + " :" + msg + "\r\n";
                    pushLine(toFd, dm);
                }
            }
        }
        return;
    }
    if (CMD == "QUIT") {
        // close from outside poll loop: just request closing by sending 0 bytes / flag
        // simpler: enqueue and let remote close, or call cleanup here by finding index.
        // Minimal: mark for close by sending "KILL" to yourself:
        (void)0;
        return;
    }



    // fallback: echo or ignore
    sendNumeric(fd, "421", cmd, "Unknown command");
    
    
}