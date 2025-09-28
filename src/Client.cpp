/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gahmed <gahmed@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 14:30:35 by gahmed            #+#    #+#             */
/*   Updated: 2025/09/28 12:53:55 by gahmed           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./inc/ClientA.hpp"


Client::Client(int socket_fd) 
    : fd(socket_fd), authenticated(false), registered(false), inbuff(""), outbuff(""), joined("") {
    // Initialize client with socket file descriptor
}

Client::~Client() {
    if (fd != -1) {
        close(fd);
    }
}

void Client::setNickname(const std::string& nick) {
    nickname = nick;
    checkRegistrationComplete();
}

void Client::setUsername(const std::string& user, const std::string& real) {
    username = user;
    realname = real;
    checkRegistrationComplete();
}

int Client::getFd() const {
    return fd;
}

std::string Client::getNickname() const {
    return nickname;
}