AM_YFLAGS = -Wall -rall -d
bin_PROGRAMS = ahci
ahci_SOURCES = ahci.c parser.c symtab.c grammar.y lex.l

AUTOMAKE_OPTIONS = dejagnu
DEJATOOL = ahci
RUNTESTFLAGS = --srcdir ${srcdir}/testsuite AHCI=${top_builddir}/ahci

