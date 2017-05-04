# Build executables and documentation; run tests.
#
# SCL; 2017.

deps_prefix = ../../extern
gr1c_include = ../../src

LEX = flex -X
LFLAGS =
YACC = bison -y
YFLAGS = -d

CC = gcc
LD = ld -r

CFLAGS = -g -Wall -pedantic -std=c99 -I$(deps_prefix)/include -Isrc
LDFLAGS = -L$(deps_prefix)/lib -lm -lcudd


printwin: printwin.c
	$(CC) -o $@ $^
