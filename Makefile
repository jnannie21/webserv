NAME = webserv

FLAGS = -std=c++98 -Wall -Wextra -Werror -g

SRCS =          sources/utils.cpp \
                sources/cpp_libft/stl_containers_operator_overloading.cpp \
                sources/get_next_line/get_next_line.cpp \
                sources/get_next_line/get_next_line_utils.cpp \
                sources/cpp_libft/get_next_word.cpp \
                sources/cpp_libft/isspace.cpp \
                sources/cpp_libft/isdigit.cpp \
                sources/cpp_libft/unsigned_number_len.cpp \
                sources/cpp_libft/atoi.cpp\
                sources/cpp_libft/tolower.cpp\
                sources/cpp_libft/is_correct_ip_octet.cpp \
                sources/cpp_libft/strtoul_base.cpp \
                sources/cpp_libft/string_to_lower.cpp \
                sources/cpp_libft/ultostr_base.cpp \
                sources/cpp_libft/stoll_base.cpp \
                sources/cpp_libft/percent_decode.cpp \
                sources/cpp_libft/toupper.cpp \
                sources/cpp_libft/string_to_upper.cpp \
\
                sources/WebServ.cpp \
                sources/WebServ_Router.cpp \
                sources/Listener.cpp \
                sources/Server.cpp\
                sources/Request.cpp \
                sources/Response.cpp \
                sources/Base64.cpp \
\
                sources/config_parsing/Config.cpp \
                sources/config_parsing/ServerContext.cpp \
                sources/config_parsing/LocationContext.cpp \
                sources/main.cpp



OBJS = $(SRCS:.cpp=.o)

MAKE_DEPS = $(SRCS:.cpp=.d)

all: $(NAME)

$(NAME): $(OBJS)
	clang++ $(FLAGS) $(OBJS) -o $(NAME)

clean:
	rm -rf $(OBJS) $(MAKE_DEPS)

fclean: clean
	rm -rf $(NAME)

re:	fclean $(NAME)

%.o: %.cpp
	clang++ $(FLAGS) -c $< -o $@  -MD

include $(wildcard $(MAKE_DEPS))

.PHONY: all clean fclean re
