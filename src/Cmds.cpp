/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cmds.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/28 19:34:50 by xhuang            #+#    #+#             */
/*   Updated: 2025/09/28 19:35:07 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"

void Server::handleCommand(int fd, std::string_view line) {
    // Simple command parser: split by space, first is command, rest are params
    std::istringstream iss(std::string(line));
    std::string cmd;
    iss >> cmd;
    if (cmd.empty()) return;

    if (cmd == "NICK") {
        std::string nick;
        iss >> nick;
        if (nick.empty()) {
            sendNumeric(fd, "431", "", "No nickname given");
            return;
        }
        if (nick2client.count(nick)) {
            sendNumeric(fd, "433", nick, "Nickname is already in use");
            return;
        }
        Client& client = fd2client[fd];
        if (!client.getNick().empty()) {
            nick2client.erase(client.getNick());
        }
        client.setNick(nick);
        nick2client[nick] = &client;
        sendNumeric(fd, "001", nick, "Welcome to the IRC server!");
    } else if (cmd == "USER") {
        std::string user, host, server, realname;
        iss >> user >> host >> server;
        std::getline(iss, realname);
        if (user.empty() || host.empty() || server.empty() || realname.empty()) {
            sendNumeric(fd, "461", cmd, "Not enough parameters");
            return;
        }
        Client& client = fd2client[fd];
        client.setUser(user);
        client.setRealName(realname.substr(1)); // skip leading space
        // For simplicity, we don't validate host/server here
    } else if (cmd == "PING") {
        std::string token;
        iss >> token;
        if (token.empty()) {
            sendNumeric(fd, "409", "", "No origin specified");
            return;
        }
        pushLine(fd, "PONG :" + token + "\r\n");
    } else if (cmd == "JOIN") {
        std::string chanName;
        iss >> chanName;
        if (chanName.empty() || chanName[0] != '#') {
            sendNumeric(fd, "476", chanName, "Invalid channel name");
            return;
        }
        Client& client = fd2client[fd];
        if (client.getNick().empty()) {
            sendNumeric(fd, "451", "", "You have not registered