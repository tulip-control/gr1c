# SCL; 7 Mar 2012.
#
#

INSTALLDIR = ~/opt/bin
CUDD_ROOT = extern/cudd-2.5.0
CUDD_LIB = $(CUDD_ROOT)/cudd/libcudd.a $(CUDD_ROOT)/mtr/libmtr.a $(CUDD_ROOT)/st/libst.a $(CUDD_ROOT)/util/libutil.a $(CUDD_ROOT)/epd/libepd.a

LEX = flex
LFLAGS = 
YACC = bison -y
YFLAGS = -d

CC = gcc
CFLAGS = -g -Wall -pedantic -ansi -I$(CUDD_ROOT)/include -DHAVE_IEEE_754 -DBSD -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8
LDFLAGS = $(CUDD_LIB) -lm


gr1c: main.o solve_operators.o solve.o ptree.o automaton.o gr1c_parse.o gr1c_scan.o
	$(CC) -o $@ $^ $(LDFLAGS)

main.o: main.c
ptree.o: ptree.c
automaton.o: automaton.c
solve_operators.o: solve_operators.c
solve.o: solve.c

gr1c_scan.o: gr1c_scan.l gr1c_parse.c
gr1c_parse.o: gr1c_parse.y


install:
	cp gr1c $(INSTALLDIR)

clean:
	rm -fv *~ *.o y.tab.h gr1c_parse.c gr1c
