# SCL; 26 Jan 2012.
#
#

LEX = flex
LFLAGS = 
YACC = bison -y
YFLAGS = -d

CC = gcc
CFLAGS = -g -Wall -pedantic
LDFLAGS = 


# Stand-alone gr1c syntax checker
gr1c_parse: gr1c_parse.o gr1c_scan.o

gr1c_scan.o: gr1c_scan.l gr1c_parse.c
gr1c_parse.o: gr1c_parse.y


clean:
	rm -fv *~ *.o y.tab.h gr1c_parse.c gr1c_parse
