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

#include "Global.hpp"
#include <algorithm>
#include <string>
#include <vector>
#include <cerrno>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Client {
    private:
        int fd;                         // File descriptor
        std::string nickname;           // User's nickname
        std::string username;           // User's username
        std::string hostname;           // Client's hostname
        bool authenticated;             // Password authentication status
        bool registered;                // Full registration status
        std::string inbuff;             // Incoming message buffer
        std::string outbuff;            // outgoing message buffer
        std::vector<std::string> joinedChannels; //JUL
        
    public:
        // Constructor/Destructor
        Client(int socket_fd);
        Client(const Client& other);
        Client& operator=(const Client& other);
        ~Client();
        
        //Getters
        int getFd() const;
        std::string getNickname() const;
        std::string getHostname() const;
        bool getAuthenticated() const;
        bool isRegistered() const;

        // Authentication methods
        void setPassOk(bool ok);
        // bool authenticate(const std::string& password);
        void setNickname(const std::string& nick);
        void setUsername(const std::string& user);
        void checkRegistrationComplete();
        void detectHostname();
        
        // Message handling
        // void appendToBuffer(const std::string& data);
        // std::vector<std::string> extractMessages();
        // void receiveOnce();
        void appendInbuff(const char* data, size_t n);
        bool extractLine(std::string& line);
        bool sendMessage(const std::string& message);
        void markForClose();
        bool flushOutput();

        // JUL
        size_t findChannelIndex(const std::string& channel) const;
        void joinChannel(const std::string& channel);
        void leaveChannel(const std::string& channel);
        std::string& getOutput();
        std::vector<std::string> getChannels() const;
        bool hasOutput() const;
};