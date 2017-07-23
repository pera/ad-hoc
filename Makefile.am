AM_YFLAGS = -Werror=all -rall -d
bin_PROGRAMS = ahci
ahci_SOURCES = src/ahci.c src/list.h src/extra.h src/parser.c src/parser.h src/symtab.c src/symtab.h src/grammar.y src/lex.l

AUTOMAKE_OPTIONS = dejagnu
DEJATOOL = ahci
TESTSUITE = $(srcdir)/testsuite
RUNTESTFLAGS = --srcdir ${TESTSUITE} AHCI=${top_builddir}/ahci

EXTRA_DIST = $(TESTSUITE)

