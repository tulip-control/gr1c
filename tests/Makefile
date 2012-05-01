# Unit and integration tests of gr1c
#
# SCL; 26 Apr 2012.


CUDD_LIB = ../$(CUDD_ROOT)/cudd/libcudd.a ../$(CUDD_ROOT)/mtr/libmtr.a ../$(CUDD_ROOT)/st/libst.a ../$(CUDD_ROOT)/util/libutil.a ../$(CUDD_ROOT)/epd/libepd.a

CFLAGS = -g -Wall -pedantic -ansi -I.. -I../$(CUDD_ROOT)/include
LDFLAGS = $(CUDD_LIB) -lm
PROGRAMS = test_automaton test_ptree test_ptree_to_BDD test_solve_support

all: $(PROGRAMS)
	./test_ptree
	./test_ptree_to_BDD
	./test_automaton
	./test_solve_support
	sh test-gr1c.sh
	@echo "============================================================\nPASSED\n"


test_ptree: test_ptree.c
	gcc $(CFLAGS) $^ ../ptree.o -o $@ $(LDFLAGS)

test_ptree_to_BDD: test_ptree_to_BDD.c
	gcc $(CFLAGS) $^ ../ptree.o -o $@ $(LDFLAGS)

test_automaton: test_automaton.c
	gcc $(CFLAGS) $^ ../automaton.o ../ptree.o -o $@ $(LDFLAGS)

test_solve_support: test_solve_support.c
	gcc $(CFLAGS) $^ ../automaton.o ../ptree.o ../solve.o ../solve_operators.o -o $@ $(LDFLAGS)

clean:
	rm -fv *~ *.o $(PROGRAMS) temp_*_dump*
