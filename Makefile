# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: mmonika <mmonika@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/09/08 15:49:16 by junjun            #+#    #+#              #
#    Updated: 2025/11/09 13:28:25 by mmonika          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #


NAME		= ircserv

CC 			= c++

CPPFLAGS	= -Wall -Wextra -Werror -std=c++17

VALGRIND	= valgrind --leak-check=full --track-origins=yes

SRC			= src/main.cpp src/Server.cpp src/Client.cpp src/Cmdhandler.cpp src/Channel.cpp src/Parser.cpp

OBJ_DIR		= obj
OBJ			= $(SRC:%.cpp=$(OBJ_DIR)/%.o)

BOT_BIN		= ircbot
BOT_SRC		= src/Bot.cpp src/Parser.cpp
BOT_OBJ		= $(BOT_SRC:%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME) 
bonus: $(BOT_BIN)

$(NAME): $(OBJ)
	@echo "\033[34m🔄 Loading....\033[0m"
	@$(CC) $(CPPFLAGS) -o $(NAME) $(OBJ)
	@echo "\033[32m🚀 Program is ready to execute\033[0m"

$(BOT_BIN): $(BOT_OBJ)
	@echo "\033[34m🔄 Loading IRCBOT....\033[0m"
	@$(CC) $(CPPFLAGS) -o $(BOT_BIN) $(BOT_OBJ)
	@echo "\033[32m🚀 IRCBOT is ready\033[0m"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir	-p	$(dir $@)
	@$(CC) $(CPPFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJ) $(BOT_OBJ)
	@echo "\033[31mObject files removed\033[0m"

fclean: clean
	@rm -f $(NAME) $(BOT_BIN)
	@rm -rf $(OBJ_DIR)
	@echo "\033[31mProgram removed\033[0m"

re: fclean all

valgrind: $(NAME)
	$(VAL) ./$(NAME)

.PHONY: all clean fclean re valgrind bonus