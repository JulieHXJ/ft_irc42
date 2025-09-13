/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Chanel.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 23:56:57 by junjun            #+#    #+#             */
/*   Updated: 2025/09/14 00:09:33 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANEL_HPP
#define CHANEL_HPP

#include <string>
#include <set>

class Chanel {
private:
std::string name;
std::string topic;
size_t maxUsers;
size_t currentUsers;

std::set<int> members; // set of client fds
std::set<int> operators; // set of operator fds
public:
Chanel(const std::string& channelName, size_t maxUsers = 100)
	: name(channelName), topic(""), maxUsers(maxUsers), currentUsers(0) {}



}
#endif // CHANEL_HPP