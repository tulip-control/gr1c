# Build executables and documentation; run tests.
#
# SCL; 2012-2015.

RELEASE=0

CORE_PROGRAMS = gr1c gr1c-rg
EXP_PROGRAMS = gr1c-patch
AUX_PROGRAMS = gr1c-autman


prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
deps_prefix = extern

# Possibly change to `cp' when `install' is unavailable
INSTALL = install

INCLUDEDIR = include
SRCDIR = src
EXPDIR = exp

LEX = flex -X
LFLAGS = 
YACC = bison -y
YFLAGS = -d

CC = gcc
LD = ld -r

CFLAGS = -Wall -pedantic -std=c99 -I$(deps_prefix)/include -I$(INCLUDEDIR)
LDFLAGS = -L$(deps_prefix)/lib -lm -lcudd

# To use and statically link with GNU Readline
#CFLAGS += -DUSE_READLINE
#LDFLAGS += -lreadline

# N.B., scripted interaction tests, which are invoked if you run "make check",
# will fail if you build gr1c with GNU Readline.


# To measure test coverage
#CFLAGS += -fprofile-arcs -ftest-coverage
#LDFLAGS += -lgcov
ifeq ($(RELEASE),0)
	GR1C_GITHASH := $(shell git describe --dirty)
	ifdef GR1C_GITHASH
		GR1C_GITHASH := "+$(GR1C_GITHASH)"
	else
		GR1C_GITHASH := ""
	endif
	CFLAGS += -g -DGR1C_DEVEL=\".dev0$(GR1C_GITHASH)\"
else
	CFLAGS += -O3
endif


core: $(CORE_PROGRAMS) $(EXP_PROGRAMS) $(AUX_PROGRAMS)
all: core

gr1c: main.o util.o logging.o interactive.o solve_support.o solve_operators.o solve.o ptree.o automaton.o automaton_io.o gr1c_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)

gr1c-rg: rg_main.o util.o patching_support.o logging.o solve_support.o solve_operators.o solve.o ptree.o automaton.o automaton_io.o rg_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)

gr1c-autman: util.o logging.o solve_support.o ptree.o autman.o automaton.o automaton_io.o gr1c_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)

gr1c-patch: grpatch.o util.o logging.o interactive.o solve_metric.o solve_support.o solve_operators.o solve.o patching.o patching_support.o patching_hotswap.o ptree.o automaton.o automaton_io.o gr1c_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)

grjit: grjit.o sim.o util.o logging.o interactive.o solve_metric.o solve_support.o solve_operators.o solve.o ptree.o automaton.o automaton_io.o gr1c_parse.o
	$(CC) -o $@ $^ $(LDFLAGS)

autman.o: aux/autman.c
	$(CC) $(CFLAGS) -c $^

grpatch.o: $(EXPDIR)/grpatch.c
	$(CC) $(CFLAGS) -c $^
grjit.o: $(EXPDIR)/grjit.c
	$(CC) $(CFLAGS) -c $^

main.o: $(SRCDIR)/main.c $(INCLUDEDIR)/common.h
	$(CC) $(CFLAGS) -c $<
rg_main.o: $(SRCDIR)/rg_main.c $(INCLUDEDIR)/common.h
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
automaton_io.o: $(SRCDIR)/automaton_io.c $(INCLUDEDIR)/common.h
	$(CC) $(CFLAGS) -c $<
interactive.o: $(SRCDIR)/interactive.c $(INCLUDEDIR)/common.h
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
rg_parse.o: $(SRCDIR)/gr1c_scan.l $(SRCDIR)/rg_parse.y gr1c_parse.o
	$(YACC) $(YFLAGS) $(SRCDIR)/rg_parse.y
	$(LEX) $<
	$(CC) $(CFLAGS) -c lex.yy.c y.tab.c
	$(LD) lex.yy.o y.tab.o -o $@

install:
	$(INSTALL) $(CORE_PROGRAMS) $(EXP_PROGRAMS) $(AUX_PROGRAMS) $(DESTDIR)$(bindir)

uninstall:
	rm -f $(DESTDIR)$(bindir)/gr1c $(DESTDIR)$(bindir)/gr1c-rg $(DESTDIR)$(bindir)/gr1c-patch

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
	-rm -f *.gcno *.gcda *.gcov

# Delete testing-related things
.PHONY: tclean
tclean:
	$(MAKE) -C tests clean

# Clean everything
clean: dclean eclean tclean
