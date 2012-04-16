# SCL; 15 Apr 2012.
#
# Unit and functional tests of gr1c

CUDD_LIB = ../$(CUDD_ROOT)/cudd/libcudd.a ../$(CUDD_ROOT)/mtr/libmtr.a ../$(CUDD_ROOT)/st/libst.a ../$(CUDD_ROOT)/util/libutil.a ../$(CUDD_ROOT)/epd/libepd.a

CFLAGS = -Wall -pedantic -ansi -I.. -I../$(CUDD_ROOT)/include
LDFLAGS = $(CUDD_LIB)
PROGRAMS = test_automaton

all: $(PROGRAMS)
	./test_automaton
	sh test-gr1c.sh
	@echo "============================================================\nPASSED\n"

test_automaton: test_automaton.c
	gcc $(CFLAGS) $^ $(LDFLAGS) ../automaton.o ../ptree.o -o $@

clean:
	rm -fv *~ *.o $(PROGRAMS)
