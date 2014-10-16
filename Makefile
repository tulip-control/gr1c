# Build executables and documentation; run tests.
#
# SCL; 2012-2014.

CORE_PROGRAMS = gr1c rg
EXP_PROGRAMS = grpatch
AUX_PROGRAMS = autman


prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

# Possibly change to `cp' when `install' is unavailable
INSTALL = install

SRCDIR = src
EXPDIR = exp
export CUDD_ROOT = extern/cudd-2.5.0
CUDD_LIB = $(CUDD_ROOT)/cudd/libcudd.a $(CUDD_ROOT)/mtr/libmtr.a $(CUDD_ROOT)/st/libst.a $(CUDD_ROOT)/util/libutil.a $(CUDD_ROOT)/epd/libepd.a
export CUDD_XCFLAGS = -mtune=native -DHAVE_IEEE_754 -DBSD -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8
CUDD_INC = -I$(CUDD_ROOT)/include

LEX = flex
LFLAGS = 
YACC = bison -y
YFLAGS = -d

CC = gcc
LD = ld -r

CFLAGS = -g -Wall -pedantic -ansi
ALL_CFLAGS = $(CFLAGS) $(CUDD_XCFLAGS) $(CUDD_INC) -Isrc
LDFLAGS = $(CUDD_LIB) -lm $(CUDD_XCFLAGS)

# To use and statically link with GNU Readline
#CFLAGS += -DUSE_READLINE
#LDFLAGS += -lreadline

# N.B., scripted interaction tests, which are invoked if you run "make check",
# will fail if you build gr1c with GNU Readline.


core: $(CORE_PROGRAMS)
exp: $(EXP_PROGRAMS)
aux: $(AUX_PROGRAMS)
all: core exp aux

gr1c: main.o util.o logging.o interactive.o solve_support.o solve_operators.o solve.o ptree.o automaton.o automaton_io.o gr1c_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)

rg: rg_main.o util.o patching_support.o logging.o solve_support.o solve_operators.o solve.o ptree.o automaton.o automaton_io.o rg_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)

autman: util.o logging.o solve_support.o ptree.o autman.o automaton.o automaton_io.o gr1c_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)

grpatch: grpatch.o util.o logging.o interactive.o solve_metric.o solve_support.o solve_operators.o solve.o patching.o patching_support.o patching_hotswap.o ptree.o automaton.o automaton_io.o gr1c_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)

grjit: grjit.o sim.o util.o logging.o interactive.o solve_metric.o solve_support.o solve_operators.o solve.o ptree.o automaton.o automaton_io.o gr1c_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)

autman.o: aux/autman.c
	$(CC) $(ALL_CFLAGS) -c $^

grpatch.o: $(EXPDIR)/grpatch.c
	$(CC) $(ALL_CFLAGS) -c $^
grjit.o: $(EXPDIR)/grjit.c
	$(CC) $(ALL_CFLAGS) -c $^

main.o: $(SRCDIR)/main.c $(SRCDIR)/common.h
	$(CC) $(ALL_CFLAGS) -c $<
rg_main.o: $(SRCDIR)/rg_main.c $(SRCDIR)/common.h
	$(CC) $(ALL_CFLAGS) -c $<
sim.o: $(SRCDIR)/sim.c
	$(CC) $(ALL_CFLAGS) -c $^
util.o: $(SRCDIR)/util.c
	$(CC) $(ALL_CFLAGS) -c $^
ptree.o: $(SRCDIR)/ptree.c
	$(CC) $(ALL_CFLAGS) -c $^
logging.o: $(SRCDIR)/logging.c
	$(CC) $(ALL_CFLAGS) -c $^
automaton.o: $(SRCDIR)/automaton.c
	$(CC) $(ALL_CFLAGS) -c $^
automaton_io.o: $(SRCDIR)/automaton_io.c $(SRCDIR)/common.h
	$(CC) $(ALL_CFLAGS) -c $<
interactive.o: $(SRCDIR)/interactive.c $(SRCDIR)/common.h
	$(CC) $(ALL_CFLAGS) -c $<
solve_metric.o: $(SRCDIR)/solve_metric.c
	$(CC) $(ALL_CFLAGS) -c $^
solve_support.o: $(SRCDIR)/solve_support.c
	$(CC) $(ALL_CFLAGS) -c $^
solve_operators.o: $(SRCDIR)/solve_operators.c
	$(CC) $(ALL_CFLAGS) -c $^
solve.o: $(SRCDIR)/solve.c
	$(CC) $(ALL_CFLAGS) -c $^
patching.o: $(SRCDIR)/patching.c
	$(CC) $(ALL_CFLAGS) -c $^
patching_support.o: $(SRCDIR)/patching_support.c
	$(CC) $(ALL_CFLAGS) -c $^
patching_hotswap.o: $(SRCDIR)/patching_hotswap.c
	$(CC) $(ALL_CFLAGS) -c $^

gr1c_parse.o: $(SRCDIR)/gr1c_scan.l $(SRCDIR)/gr1c_parse.y
	$(YACC) $(YFLAGS) $(SRCDIR)/gr1c_parse.y
	$(LEX) $<
	$(CC) $(ALL_CFLAGS) -c lex.yy.c y.tab.c
	$(LD) lex.yy.o y.tab.o -o $@
rg_parse.o: $(SRCDIR)/gr1c_scan.l $(SRCDIR)/rg_parse.y
	$(YACC) $(YFLAGS) $(SRCDIR)/rg_parse.y
	$(LEX) $<
	$(CC) $(ALL_CFLAGS) -c lex.yy.c y.tab.c
	$(LD) lex.yy.o y.tab.o -o $@

install:
	$(INSTALL) $(CORE_PROGRAMS) $(DESTDIR)$(bindir)

.PHONY: expinstall
expinstall:
	$(INSTALL) grpatch $(DESTDIR)$(bindir)

uninstall:
	rm -f $(DESTDIR)$(bindir)/gr1c $(DESTDIR)$(bindir)/rg $(DESTDIR)$(bindir)/grpatch

check: $(CORE_PROGRAMS) $(EXP_PROGRAMS)
	$(MAKE) -C tests CC=$(CC)

.PHONY: doc
doc:
	@(cd doc; doxygen; cd ..)

# Delete built documentation
.PHONY: dclean
dclean:
	-rm -fr doc/build/*

# Delete only executables and corresponding object code
.PHONY: eclean
eclean:
	-rm -f *~ *.o y.tab.h y.tab.c lex.yy.c $(CORE_PROGRAMS) $(EXP_PROGRAMS) $(AUX_PROGRAMS)

# Delete testing-related things
.PHONY: tclean
tclean:
	$(MAKE) -C tests clean

# Clean everything
clean: dclean eclean tclean
