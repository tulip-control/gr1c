# SCL; 3 July 2012.
#
#

INSTALLDIR = ~/opt/bin
export CUDD_ROOT = extern/cudd-2.5.0
CUDD_LIB = $(CUDD_ROOT)/cudd/libcudd.a $(CUDD_ROOT)/mtr/libmtr.a $(CUDD_ROOT)/st/libst.a $(CUDD_ROOT)/util/libutil.a $(CUDD_ROOT)/epd/libepd.a

LEX = flex
LFLAGS = 
YACC = bison -y
YFLAGS = -d

CC = gcc
CFLAGS = -g -Wall -pedantic -ansi -I$(CUDD_ROOT)/include -DHAVE_IEEE_754 -DBSD -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8
LDFLAGS = $(CUDD_LIB) -lm

# To use and statically link with GNU Readline
#CFLAGS += -DUSE_READLINE
#LDFLAGS += -lreadline


gr1c: main.o interactive.o solve_operators.o solve.o patching.o ptree.o automaton.o automaton_io.o gr1c_parse.o gr1c_scan.o
	$(CC) -o $@ $^ $(LDFLAGS)

# gr1patch: gr1patch.c automaton.o
# 	$(CC) -o $@ $^ $(LDFLAGS)

main.o: main.c
ptree.o: ptree.c
automaton.o: automaton.c
automaton_io.o: automaton_io.c
interactive.o: interactive.c
solve_operators.o: solve_operators.c
solve.o: solve.c
patching.o: patching.c

gr1c_scan.o: gr1c_scan.l gr1c_parse.c
gr1c_parse.o: gr1c_parse.y


install:
	cp gr1c $(INSTALLDIR)

uninstall:
	rm $(INSTALLDIR)/gr1c

check: gr1c
	$(MAKE) -C tests

# Generate PNG images from DOT files in local directory
png:
	@(for k in *.dot; do \
		echo $$k; \
		dot -Tpng -O $$k; \
	done)

.PHONY: doc
doc:
	@(cd doc; doxygen; cd ..)

clean:
	rm -fv *~ *.o y.tab.h gr1c_parse.c gr1c
	rm -fr doc/build/*
	$(MAKE) -C tests clean
