/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gahmed <gahmed@student.42heilbronn.de>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 00:53:12 by junjun            #+#    #+#             */
/*   Updated: 2025/11/16 23:04:10 by gahmed           ###   ########.fr       */
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
INVITE bob #tea\r\n
KICK #tea bob :stop spamming\r\n
PRIVMSG #tea :hello everyone\r\n
PRIVMSG bob :hi bob\r\n
MODE #tea +i\r\n
QUIT :Client exiting\r\n
 */

struct IRCMessage
{
    std::string prefix;     // server or nick!user@host or empty
    std::string command;    // NICK, USER, JOIN, etc.
    std::vector<std::string> params;    // Command parameters and trailing
    std::string trailing;   // Trailing (after " :")
};

IRCMessage parseMessage(const std::string& raw);

#endif