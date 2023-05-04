NAME = ft_yacc

SRCS =	scanner.yy.cpp \
 		Scanner.cpp \
		Parser.cpp \
		LALR.cpp \
		main.cpp \
		utils.cpp \
		CommandLine.cpp \
		Generator.cpp \
		Serializer.cpp \
		SyntaxError.cpp

SRCS_DIR = srcs

OBJS_DIR = .objs

INCLUDE_DIR = srcs

CXXFLAGS = -g3 -std=c++20 -D SKELETONS_PATHS=\"$(shell pwd)/skeletons\"

LDFLAGS =

LEX=../ft_lex/ft_lex

include template_cpp.mk
