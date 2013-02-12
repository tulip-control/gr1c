# Build gr1c and rg executables, build documentation, run tests.
#
# SCL; 2012, 2013.


INSTALLDIR = /usr/local/bin
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


all: gr1c rg

gr1c: main.o sim.o util.o logging.o interactive.o solve_metric.o solve_support.o solve_operators.o solve.o patching.o patching_support.o ptree.o automaton.o automaton_io.o gr1c_parse.o gr1c_scan.o
	$(CC) -o $@ $^ $(LDFLAGS)

rg: rg_main.o util.o patching_support.o logging.o solve_support.o solve_operators.o solve.o ptree.o automaton.o automaton_io.o rg_parse.o gr1c_scan.o
	$(CC) -o $@ $^ $(LDFLAGS)

main.o: main.c common.h
rg_main.o: rg_main.c common.h
sim.o: sim.c
util.o: util.c
ptree.o: ptree.c
logging.o: logging.c
automaton.o: automaton.c
automaton_io.o: automaton_io.c common.h
interactive.o: interactive.c common.h
solve_metric.o: solve_metric.c
solve_support.o: solve_support.c
solve_operators.o: solve_operators.c
solve.o: solve.c
patching.o: patching.c
patching_support.o: patching_support.c

gr1c_scan.o: gr1c_scan.l gr1c_parse.c rg_parse.y
gr1c_parse.o: gr1c_parse.y
rg_parse.o: rg_parse.y


install:
	cp gr1c rg $(INSTALLDIR)/

uninstall:
	rm $(INSTALLDIR)/gr1c $(INSTALLDIR)/rg

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

# Clean everything
clean:
	rm -fv *~ *.o y.tab.h gr1c_parse.c rg_parse.c gr1c rg
	rm -fr doc/build/*
	$(MAKE) -C tests clean

# Delete only executables and corresponding object code
.PHONY: eclean
eclean:
	rm -fv *~ *.o y.tab.h gr1c_parse.c rg_parse.c gr1c rg

# Delete testing-related things
.PHONY: tclean
tclean:
	$(MAKE) -C tests clean
