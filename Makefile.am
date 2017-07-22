AM_YFLAGS = -Wall -rall -d
bin_PROGRAMS = ahci
ahci_SOURCES = ahci.c list.h extra.h parser.c parser.h symtab.c symtab.h grammar.y lex.l

AUTOMAKE_OPTIONS = dejagnu
DEJATOOL = ahci
RUNTESTFLAGS = --srcdir ${srcdir}/testsuite AHCI=${top_builddir}/ahci

