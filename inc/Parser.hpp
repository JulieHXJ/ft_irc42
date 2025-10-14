/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 00:53:12 by junjun            #+#    #+#             */
/*   Updated: 2025/10/14 22:05:01 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

/**
 * IRC message format: [':' <prefix> ' '] <command> <params> [ ' :' <trailing> ] CRLF
NICK alice\r\n
USER alice 0 * :Alice Liddell\r\n
JOIN #tea\r\n
TOPIC #tea :Tea time at 5pm\r\n
PART #tea :gotta run\r\n
INVITE #tea bob\r\n
KICK #tea bob :stop spamming\r\n
PRIVMSG #tea :hello everyone\r\n
MODE #tea +it\r\n
QUIT :Client exiting\r\n
 */
struct IRCmessage {
    std::string prefix;  // server or nick!user@host or empty
    std::string command;             // NICK, USER, JOIN, etc.
    std::vector<std::string> params; // Command parameters and trailing
};


IRCmessage parseLine(const std::string &rawLine);

#endif
