NAME = ft_yacc

SRCS = Scanner.cpp \
		scanner.yy.cpp \
		Parser.cpp \
		LALR.cpp \
		main.cpp \
		utils.cpp \
		CommandLine.cpp \
		Generator.cpp \
		SyntaxError.cpp

SRCS_DIR = srcs

OBJS_DIR = .objs

INCLUDE_DIR = srcs

CXXFLAGS = -g3 -std=c++20

LDFLAGS =

LEX=../ft_lex/ft_lex

include template_cpp.mk
