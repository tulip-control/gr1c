# Unit and integration tests of gr1c
#
# SCL; 2012, 2013.


CUDD_LIB = ../$(CUDD_ROOT)/cudd/libcudd.a ../$(CUDD_ROOT)/mtr/libmtr.a ../$(CUDD_ROOT)/st/libst.a ../$(CUDD_ROOT)/util/libutil.a ../$(CUDD_ROOT)/epd/libepd.a

CFLAGS = -g -Wall -pedantic -ansi -I../src -I../$(CUDD_ROOT)/include
LDFLAGS = $(CUDD_LIB) -lm
PROGRAMS = test_logging test_automaton test_automaton_io test_ptree test_ptree_to_BDD test_solve_support test_patching

all: $(PROGRAMS)
	./test_logging
	./test_ptree
	./test_ptree_to_BDD
	./test_automaton
	./test_automaton_io
	./test_solve_support
	./test_patching
	sh test-gr1c.sh
	@echo "============================================================\nPASSED\n"


COMMON_BINS = ../util.o ../automaton.o ../automaton_io.o ../ptree.o ../solve_support.o ../logging.o

test_logging: test_logging.c
	gcc $(CFLAGS) $^ ../logging.o -o $@ $(LDFLAGS)

test_ptree: test_ptree.c
	gcc $(CFLAGS) $^ ../ptree.o -o $@ $(LDFLAGS)

test_ptree_to_BDD: test_ptree_to_BDD.c
	gcc $(CFLAGS) $^ ../ptree.o -o $@ $(LDFLAGS)

test_automaton: test_automaton.c
	gcc $(CFLAGS) $^ $(COMMON_BINS) -o $@ $(LDFLAGS)

test_automaton_io: test_automaton_io.c
	gcc $(CFLAGS) $^ $(COMMON_BINS) -o $@ $(LDFLAGS)

test_solve_support: test_solve_support.c
	gcc $(CFLAGS) $^ $(COMMON_BINS) -o $@ $(LDFLAGS)

test_patching: test_patching.c
	gcc $(CFLAGS) $^ $(COMMON_BINS) ../patching.o ../patching_support.o -o $@ $(LDFLAGS)

clean:
	rm -fv *~ *.o $(PROGRAMS) temp_*_dump*
	rm -rfv *.dSYM
