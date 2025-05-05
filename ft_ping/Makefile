NAME = ft_ping
CC = gcc
CFLAGS = -Wall -Wextra -Werror
SRCS = src/ft_ping.c src/utils.c src/print_mess.c
OBJS = $(SRCS:%.c=%.o)
INCLUDES = -Iinc

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

	@echo "Setting SUID bit for root privileges"
	@sudo chown root $(NAME)
	@sudo chmod u+s $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
