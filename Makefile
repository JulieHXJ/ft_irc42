# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: junjun <junjun@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/09/08 15:42:03 by junjun            #+#    #+#              #
#    Updated: 2025/10/17 17:14:05 by junjun           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME        = ircserv

CXX         = c++

CXXFLAGS    = -Wall -Wextra -Werror -std=c++17

VALGRIND	= valgrind --leak-check=full --track-origins=yes

SRCS		= src/main.cpp src/Server.cpp src/Client.cpp src/Cmdhandler.cpp \
			  src/Channel.cpp src/Parser.cpp 


OBJS		= $(SRCS:.cpp=.o)

BOT_BIN     = ircbot              # lowercase to match the source name
BOT_SRCS    = src/Bot.cpp src/Parser.cpp   # link parser into the bot
BOT_OBJS    = $(BOT_SRCS:.cpp=.o)

all: $(NAME) $(BOT_BIN)

$(NAME): $(OBJS)
	@echo "\033[34m🔄 Loading....\033[0m"
	@$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)
	@echo "\033[32m🚀 Program is ready to execute\033[0m"

$(BOT_BIN): $(BOT_OBJS)
	@echo "\033[34m🔄 Loading IRCBOT....\033[0m"
	@$(CXX) $(CXXFLAGS) $(BOT_OBJS) -o $(BOT_BIN)
	@echo "\033[32m🚀 IRCBOT is ready\033[0m"

%.o:	 %.cpp
		$(CXX) $(CXXFLAGS) -Iinc -c $< -o $@
	
clean:
	@rm -f $(OBJS) $(BOT_OBJS)
	@echo "\033[31mObject files removed\033[0m"
	
fclean:		clean
	@rm -f $(NAME) $(BOT_BIN)
	@echo "\033[31mProgram removed\033[0m"

re:		fclean 
		@$(MAKE) all

valgrind: $(NAME)
	$(VAL) ./$(NAME)

.PHONY: all clean fclean re valgrind bonus