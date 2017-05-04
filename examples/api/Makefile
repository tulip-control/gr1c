# Build executables and documentation; run tests.
#
# SCL; 2017.

deps_prefix = ../../extern
gr1c_include = ../../src

LEX = flex -X
LFLAGS =
YACC = bison -y
YFLAGS = -d

CC = gcc
LD = ld -r

CFLAGS = -g -Wall -pedantic -std=c99 -I$(deps_prefix)/include -I$(gr1c_include)
LDFLAGS = -L$(deps_prefix)/lib -lm -lcudd


printwin: printwin.c solve.o util.o logging.o ptree.o solve_operators.o solve_support.o automaton.o gr1c_parse.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)


solve.o: $(gr1c_include)/solve.c
	$(CC) $(CFLAGS) -c $^
util.o: $(gr1c_include)/util.c
	$(CC) $(CFLAGS) -c $^
logging.o: $(gr1c_include)/logging.c
	$(CC) $(CFLAGS) -c $^
ptree.o: $(gr1c_include)/ptree.c
	$(CC) $(CFLAGS) -c $^
solve_operators.o: $(gr1c_include)/solve_operators.c
	$(CC) $(CFLAGS) -c $^
solve_support.o: $(gr1c_include)/solve_support.c
	$(CC) $(CFLAGS) -c $^
automaton.o: $(gr1c_include)/automaton.c
	$(CC) $(CFLAGS) -c $^
gr1c_parse.o: $(gr1c_include)/gr1c_scan.l $(gr1c_include)/gr1c_parse.y
	$(YACC) $(YFLAGS) $(gr1c_include)/gr1c_parse.y
	$(LEX) $<
	$(CC) $(CFLAGS) -c lex.yy.c y.tab.c
	$(LD) lex.yy.o y.tab.o -o $@

clean:
	-rm -f *~ *.o y.tab.h y.tab.c lex.yy.c
