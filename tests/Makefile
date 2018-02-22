# Unit and integration tests of gr1c
#
# SCL; 2012, 2013.

deps_prefix = ../extern
INCLUDEDIR = include

CFLAGS = -g -Wall -pedantic -std=c99 -I$(deps_prefix)/include -I../$(INCLUDEDIR)
LDFLAGS = -L$(deps_prefix)/lib -lm -lcudd

# To measure test coverage
#CFLAGS += -fprofile-arcs -ftest-coverage
#LDFLAGS += -lgcov

PROGRAMS = test_util test_logging test_automaton test_aut_prune_deadends test_aut_aut_load test_aut_aut_dump test_ptree test_ptree_to_BDD test_bitblasting test_solve_support test_patching

all: $(PROGRAMS)
	./test_logging
	./test_ptree
	./test_ptree_to_BDD
	./test_bitblasting
	./test_automaton
	./test_aut_prune_deadends
	./test_aut_aut_load
	./test_aut_aut_dump
	./test_solve_support
	./test_patching
	./test_util
	sh test-gr1c.sh
	sh test-grpatch.sh
	sh test-cli.sh
	sh test-verification.sh
	@echo "============================================================\nPASSED\n"


COMMON_BINS = ../util.o ../automaton.o ../automaton_io.o ../ptree.o ../solve_support.o ../logging.o

test_util: test_util.c
	$(CC) $(CFLAGS) $^ $(COMMON_BINS) -o $@ $(LDFLAGS)

test_logging: test_logging.c
	$(CC) $(CFLAGS) $^ ../logging.o -o $@ $(LDFLAGS)

test_ptree: test_ptree.c
	$(CC) $(CFLAGS) $^ ../ptree.o -o $@ $(LDFLAGS)

test_ptree_to_BDD: test_ptree_to_BDD.c
	$(CC) $(CFLAGS) $^ ../ptree.o -o $@ $(LDFLAGS)

test_bitblasting: test_bitblasting.c
	$(CC) $(CFLAGS) $^ $(COMMON_BINS) -o $@ $(LDFLAGS)

test_automaton: test_automaton.c
	$(CC) $(CFLAGS) $^ $(COMMON_BINS) -o $@ $(LDFLAGS)

test_aut_prune_deadends: test_aut_prune_deadends.c
	$(CC) $(CFLAGS) $^ $(COMMON_BINS) -o $@ $(LDFLAGS)

test_aut_aut_load: test_aut_aut_load.c
	$(CC) $(CFLAGS) $^ $(COMMON_BINS) -o $@ $(LDFLAGS)

test_aut_aut_dump: test_aut_aut_dump.c
	$(CC) $(CFLAGS) $^ $(COMMON_BINS) -o $@ $(LDFLAGS)

test_solve_support: test_solve_support.c
	$(CC) $(CFLAGS) $^ $(COMMON_BINS) -o $@ $(LDFLAGS)

test_patching: test_patching.c
	$(CC) $(CFLAGS) $^ $(COMMON_BINS) ../patching.o ../patching_support.o -o $@ $(LDFLAGS)

clean:
	-rm -f *~ *.o $(PROGRAMS) temp_*_dump* dump*
	-rm -fr *.dSYM
	-rm -f *.gcno *.gcda *.gcov
	-rm -f pan* *.aut *.aut.pml *.aut.pml.trail
	-rm -f tmp.*
