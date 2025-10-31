/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 01:00:39 by junjun            #+#    #+#             */
/*   Updated: 2025/10/10 16:42:46 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Parser.hpp"
#include <cctype>

//split by space, except the last param which start with ':' 
std::vector<std::string> split_param(const std::string &param) {
	std::vector<std::string> result;
	size_t i = 0, n = param.size();
	while (i < n) {
		// skip leading spaces
		while (i < n && std::isspace(param[i])) ++i;
		if (i >= n) break;
		if (param[i] == ':') {
			result.push_back(param.substr(i + 1));// last param, take the rest of the string
			break;
		}

		// if find next space
		size_t nextSpace = param.find(' ', i);
		if (nextSpace == std::string::npos) {
            result.push_back(param.substr(i));
            break;
        } else {
            result.push_back(param.substr(i, nextSpace - i));
            i = nextSpace + 1;
        }
	}
	return result;
}

std::string toUpper(const std::string &s) {
	std::string res = s;
	for (size_t i = 0; i < res.size(); ++i) {
		if (!std::isupper(res[i]) && !std::isdigit(res[i]))
		{
			res[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(res[i])));
		}
	}
	return res;
}

/**
 * @brief Parse the raw message line from client inbuff into prefix, command, params.
 * @param rawLine The raw message line (without "\r\n")
 */
IRCmessage parseLine(const std::string &rawLine) {
    IRCmessage msg;
    std::string s = rawLine;
	size_t i = 0;
	
    // Parse prefix
    if (!s.empty() && s[0] == ':') {
        size_t space = s.find(' ');
        if (space == std::string::npos) return msg;
        msg.prefix = s.substr(1, space - 1);
        i = space + 1;
    }
    // Parse command
	while (i < s.size() && s[i] == ' ') ++i;
	size_t space = s.find(' ', i);
	if (space == std::string::npos) {
		msg.command = toUpper(s.substr(i));
		return msg; // no params
	} else {
		msg.command = toUpper(s.substr(i, space - i));
		i = space + 1;
	}

    // Parse params and trailing
    msg.params = split_param(s.substr(i));
    return msg;
}
