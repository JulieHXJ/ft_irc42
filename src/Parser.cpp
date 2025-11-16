/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gahmed <gahmed@student.42heilbronn.de>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 01:00:39 by junjun            #+#    #+#             */
/*   Updated: 2025/11/16 23:11:09 by gahmed           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Parser.hpp"
#include <cctype>

std::string toUpper(const std::string &s) {
	std::string res = s;
	for (size_t i = 0; i < res.size(); ++i) {
		res[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(res[i])));
	}
	return res;
}

IRCMessage parseMessage(const std::string& raw)
{
	IRCMessage msg;
	const std::string& s = raw;
	size_t i = 0, n = s.size();
	
	if (i < n && s[i] == ':') {
		size_t space = s.find(' ', i);
		if (space == std::string::npos) return msg;
		msg.prefix = s.substr(1, space - 1);
		i = space + 1;
	}
	
	while (i < n && s[i] == ' ') ++i;
	if (i >= n) return msg;
	
	size_t cmdEnd = s.find(' ', i);
	if (cmdEnd == std::string::npos) {
		msg.command = toUpper(s.substr(i));
		return msg;
	}
	msg.command = toUpper(s.substr(i, cmdEnd - i));
	i = cmdEnd + 1;
	
	while (i < n && s[i] == ' ') ++i;
	while (i < n) {
		if (s[i] == ':') {
			msg.trailing = s.substr(i + 1);
			break;
		}
		size_t nextSpace = s.find(' ', i);
		if (nextSpace == std::string::npos) {
			msg.params.push_back(s.substr(i));
			break;
		} else {
			msg.params.push_back(s.substr(i, nextSpace - i));
			i = nextSpace + 1;
			while (i < n && s[i] == ' ') ++i;
		}
	}
	return msg;
}
