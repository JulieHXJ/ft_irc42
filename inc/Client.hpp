/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientA.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gahmed <gahmed@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 14:30:35 by gahmed            #+#    #+#             */
/*   Updated: 2025/09/28 12:46:55 by gahmed           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// IRC Message structure
struct IRCMessage {
    std::string prefix;
    std::string command;
    std::vector<std::string> parameters;
    std::string trailing;
};

class Client {
    private:
        int fd;                         // File descriptor
        std::string nickname;           // User's nickname
        std::string username;           // User's username
        std::string realname;           // User's real name
        std::string hostname;           // Client's hostname
        bool authenticated;             // Password authentication status
        bool registered;                // Full registration status
        std::string inbuff;             // Incoming message buffer
        std::string outbuff;            // outgoing message buffer
        std::string joined;             // channels joined
        
    public:
        // Constructor/Destructor
        Client(int socket_fd);
        ~Client();
        
        // Authentication methods
        bool authenticate(const std::string& password);
        void setNickname(const std::string& nick);
        void setUsername(const std::string& user, const std::string& real);
        void checkRegistrationComplete();

        
        // Message handling
        void appendToBuffer(const std::string& data);
        std::vector<std::string> extractMessages();
        IRCMessage parseMessage(const std::string& raw);
        void receiveOnce();
        void detectHostname();
        bool sendMessage(const std::string& message);
        bool flushOutput();
        
        // Getters/Setters
        int getFd() const;
        std::string getNickname() const;
        bool isAuthenticated() const;
        bool isRegistered() const;
};