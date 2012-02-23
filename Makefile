# SCL; 5 Feb 2012.
#
#

CUDD_ROOT = extern/cudd-2.5.0
CUDD_LIB = $(CUDD_ROOT)/cudd/libcudd.a $(CUDD_ROOT)/mtr/libmtr.a $(CUDD_ROOT)/st/libst.a $(CUDD_ROOT)/util/libutil.a $(CUDD_ROOT)/epd/libepd.a

LEX = flex
LFLAGS = 
YACC = bison -y
YFLAGS = -d

CC = gcc
CFLAGS = -g -Wall -pedantic -I$(CUDD_ROOT)/include
LDFLAGS = $(CUDD_LIB)


gr1c: main.o solve.o ptree.o gr1c_parse.o gr1c_scan.o
	$(CC) -o $@ $(LDFLAGS) $^

main.o: main.c
ptree.o: ptree.c
solve.o: solve.c

gr1c_scan.o: gr1c_scan.l gr1c_parse.c
gr1c_parse.o: gr1c_parse.y


clean:
	rm -fv *~ *.o y.tab.h gr1c_parse.c gr1c
