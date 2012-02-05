# SCL; 4 Feb 2012.
#
#

LEX = flex
LFLAGS = 
YACC = bison -y
YFLAGS = -d

CC = gcc
CFLAGS = -g -Wall -pedantic -I/opt/local/include/cudd
LDFLAGS = -L/opt/local/lib/cudd -lcudd -lmtr -lst -lutil


gr1c: main.o ptree.o gr1c_parse.o gr1c_scan.o
	$(CC) -o $@ $(LDFLAGS) $^

main.o: main.c
ptree.o: ptree.c

gr1c_scan.o: gr1c_scan.l gr1c_parse.c
gr1c_parse.o: gr1c_parse.y


clean:
	rm -fv *~ *.o y.tab.h gr1c_parse.c gr1c
