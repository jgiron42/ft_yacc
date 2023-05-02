NAME ?= $(shell basename $$(pwd))

SRCS_DIR ?= .

OBJS_DIR ?= obj

INCLUDE_DIR ?= .

SRCS ?= $(wildcard *.c) $(wildcard *.cpp)

OBJS_DIR_TREE := $(sort $(addprefix ${OBJS_DIR}/, $(dir ${SRCS})))

LIB_ARG = $(foreach path, $(LIBS), -L $(dir $(path)) -l $(notdir $(path)))

INCLUDE_ARG = $(addprefix -I ,${INCLUDE_DIR}) $(addprefix -I, $(dir ${LIBS}))

OBJS := $(SRCS:%.c=${OBJS_DIR}/%.o)

IDEPS := $(SRCS:%.c=${OBJS_DIR}/%.d)

CFLAGS ?= -Wall -Werror -Wextra

all: $(NAME)

$(NAME): ${OBJS_DIR} ${OBJS_DIR_TREE} $(OBJS)
		ar rc $(NAME) $(OBJS)
		ranlib $(NAME)

${OBJS_DIR}:
	mkdir -p $@
	[ -f .gitignore ] && (grep '^'"$@"'$$' .gitignore 2>&1 >/dev/null || echo "$@" >> .gitignore) || true

${OBJS_DIR_TREE}:
	mkdir -p $@

${OBJS_DIR}/%.o: ${SRCS_DIR}/%.c
	$(CC) $(CFLAGS) ${INCLUDE_ARG} -MMD -c $< -o $@

clean:
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: clean all fclean re

.PRECIOUS: ${OBJS_DIR}/%.d

-include ${IDEPS}