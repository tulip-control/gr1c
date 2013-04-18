# Build executables and documentation; run tests.
#
# SCL; 2012, 2013.

CORE_PROGRAMS = gr1c rg
EXP_PROGRAMS = grjit grpatch


INSTALLDIR = /usr/local/bin
SRCDIR = src
EXPDIR = exp
export CUDD_ROOT = extern/cudd-2.5.0
CUDD_LIB = $(CUDD_ROOT)/cudd/libcudd.a $(CUDD_ROOT)/mtr/libmtr.a $(CUDD_ROOT)/st/libst.a $(CUDD_ROOT)/util/libutil.a $(CUDD_ROOT)/epd/libepd.a

LEX = flex
LFLAGS = 
YACC = bison -y
YFLAGS = -d

CC = gcc
CFLAGS = -g -Wall -pedantic -ansi -I$(CUDD_ROOT)/include -Isrc -DHAVE_IEEE_754 -DBSD -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8
LDFLAGS = $(CUDD_LIB) -lm
LD = ld -r

# To use and statically link with GNU Readline
#CFLAGS += -DUSE_READLINE
#LDFLAGS += -lreadline

# N.B., scripted interaction tests, which are invoked if you run "make check",
# will fail if you build gr1c with GNU Readline.


core: $(CORE_PROGRAMS)
exp: $(EXP_PROGRAMS)
all: core exp

gr1c: main.o util.o logging.o interactive.o solve_support.o solve_operators.o solve.o ptree.o automaton.o automaton_io.o gr1c_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)

rg: rg_main.o util.o patching_support.o logging.o solve_support.o solve_operators.o solve.o ptree.o automaton.o automaton_io.o rg_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)

grpatch: grpatch.o util.o logging.o interactive.o solve_metric.o solve_support.o solve_operators.o solve.o patching.o patching_support.o patching_hotswap.o ptree.o automaton.o automaton_io.o gr1c_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)

grjit: grjit.o sim.o util.o logging.o interactive.o solve_metric.o solve_support.o solve_operators.o solve.o ptree.o automaton.o automaton_io.o gr1c_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)


grpatch.o: $(EXPDIR)/grpatch.c
	$(CC) $(CFLAGS) -c $^
grjit.o: $(EXPDIR)/grjit.c
	$(CC) $(CFLAGS) -c $^

main.o: $(SRCDIR)/main.c $(SRCDIR)/common.h
	$(CC) $(CFLAGS) -c $<
rg_main.o: $(SRCDIR)/rg_main.c $(SRCDIR)/common.h
	$(CC) $(CFLAGS) -c $<
sim.o: $(SRCDIR)/sim.c
	$(CC) $(CFLAGS) -c $^
util.o: $(SRCDIR)/util.c
	$(CC) $(CFLAGS) -c $^
ptree.o: $(SRCDIR)/ptree.c
	$(CC) $(CFLAGS) -c $^
logging.o: $(SRCDIR)/logging.c
	$(CC) $(CFLAGS) -c $^
automaton.o: $(SRCDIR)/automaton.c
	$(CC) $(CFLAGS) -c $^
automaton_io.o: $(SRCDIR)/automaton_io.c $(SRCDIR)/common.h
	$(CC) $(CFLAGS) -c $<
interactive.o: $(SRCDIR)/interactive.c $(SRCDIR)/common.h
	$(CC) $(CFLAGS) -c $<
solve_metric.o: $(SRCDIR)/solve_metric.c
	$(CC) $(CFLAGS) -c $^
solve_support.o: $(SRCDIR)/solve_support.c
	$(CC) $(CFLAGS) -c $^
solve_operators.o: $(SRCDIR)/solve_operators.c
	$(CC) $(CFLAGS) -c $^
solve.o: $(SRCDIR)/solve.c
	$(CC) $(CFLAGS) -c $^
patching.o: $(SRCDIR)/patching.c
	$(CC) $(CFLAGS) -c $^
patching_support.o: $(SRCDIR)/patching_support.c
	$(CC) $(CFLAGS) -c $^
patching_hotswap.o: $(SRCDIR)/patching_hotswap.c
	$(CC) $(CFLAGS) -c $^

gr1c_parse.o: $(SRCDIR)/gr1c_scan.l $(SRCDIR)/gr1c_parse.y
	$(YACC) $(YFLAGS) $(SRCDIR)/gr1c_parse.y
	$(LEX) $<
	$(CC) $(CFLAGS) -c lex.yy.c y.tab.c
	$(LD) lex.yy.o y.tab.o -o $@
rg_parse.o: $(SRCDIR)/gr1c_scan.l $(SRCDIR)/rg_parse.y
	$(YACC) $(YFLAGS) $(SRCDIR)/rg_parse.y
	$(LEX) $<
	$(CC) $(CFLAGS) -c lex.yy.c y.tab.c
	$(LD) lex.yy.o y.tab.o -o $@

install:
	cp $(CORE_PROGRAMS) $(INSTALLDIR)/

.PHONY: expinstall
expinstall:
	cp grpatch $(INSTALLDIR)/

uninstall:
	rm -f $(INSTALLDIR)/gr1c $(INSTALLDIR)/rg $(INSTALLDIR)/grpatch

check: $(CORE_PROGRAMS) $(EXP_PROGRAMS)
	$(MAKE) -C tests

.PHONY: doc
doc:
	@(cd doc; doxygen; cd ..)

# Clean everything
clean:
	rm -fv *~ *.o y.tab.h y.tab.c lex.yy.c $(CORE_PROGRAMS) $(EXP_PROGRAMS)
	rm -fr doc/build/*
	$(MAKE) -C tests clean

# Delete built documentation
.PHONY: dclean
dclean:
	rm -fr doc/build/*

# Delete only executables and corresponding object code
.PHONY: eclean
eclean:
	rm -fv *~ *.o y.tab.h y.tab.c lex.yy.c $(CORE_PROGRAMS) $(EXP_PROGRAMS)

# Delete testing-related things
.PHONY: tclean
tclean:
	$(MAKE) -C tests clean
