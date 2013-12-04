# Unit and integration tests of gr1c
#
# SCL; 2012, 2013.


CUDD_LIB = ../$(CUDD_ROOT)/cudd/libcudd.a ../$(CUDD_ROOT)/mtr/libmtr.a ../$(CUDD_ROOT)/st/libst.a ../$(CUDD_ROOT)/util/libutil.a ../$(CUDD_ROOT)/epd/libepd.a
CUDD_INC = -I../$(CUDD_ROOT)/include

CFLAGS = -g -Wall -pedantic -ansi
ALL_CFLAGS = $(CFLAGS) $(CUDD_XCFLAGS) $(CUDD_INC) -I../src
LDFLAGS = $(CUDD_LIB) -lm $(CUDD_XCFLAGS)
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
	sh test-grpatch.sh
	@echo "============================================================\nPASSED\n"


COMMON_BINS = ../util.o ../automaton.o ../automaton_io.o ../ptree.o ../solve_support.o ../logging.o

test_logging: test_logging.c
	$(CC) $(ALL_CFLAGS) $^ ../logging.o -o $@ $(LDFLAGS)

test_ptree: test_ptree.c
	$(CC) $(ALL_CFLAGS) $^ ../ptree.o -o $@ $(LDFLAGS)

test_ptree_to_BDD: test_ptree_to_BDD.c
	$(CC) $(ALL_CFLAGS) $^ ../ptree.o -o $@ $(LDFLAGS)

test_automaton: test_automaton.c
	$(CC) $(ALL_CFLAGS) $^ $(COMMON_BINS) -o $@ $(LDFLAGS)

test_automaton_io: test_automaton_io.c
	$(CC) $(ALL_CFLAGS) $^ $(COMMON_BINS) -o $@ $(LDFLAGS)

test_solve_support: test_solve_support.c
	$(CC) $(ALL_CFLAGS) $^ $(COMMON_BINS) -o $@ $(LDFLAGS)

test_patching: test_patching.c
	$(CC) $(ALL_CFLAGS) $^ $(COMMON_BINS) ../patching.o ../patching_support.o -o $@ $(LDFLAGS)

clean:
	-rm -f *~ *.o $(PROGRAMS) temp_*_dump*
	-rm -fr *.dSYM
