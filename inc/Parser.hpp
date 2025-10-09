/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 00:53:12 by junjun            #+#    #+#             */
/*   Updated: 2025/10/09 12:42:37 by junjun           ###   ########.fr       */
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
PRIVMSG #tea :hello everyone\r\n
TOPIC #tea :Tea time at 5pm\r\n
MODE #tea +it\r\n
KICK #tea bob :stop spamming\r\n
PART #tea :gotta run\r\n
QUIT :Client exiting\r\n
 */
struct Message {
    std::string prefix;  // server or nick!user@host or empty
    std::string command;
    std::vector<std::string> params;
};

Message parseLine(const std::string &rawLine);

#endif
