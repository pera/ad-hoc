CFLAGS += -Wall -Wno-unused-variable -Wno-unused-function -std=gnu11

all: clean ahci

.PHONY: clean
clean:
	rm -f grammar.tab.* grammar.output lex.yy.c *.o ahci

grammar.tab.c: grammar.y
	 bison -Wall -rall -d $<

lex.yy.c: lex.l grammar.tab.c
	 flex $^

parser.o: parser.c grammar.tab.c lex.yy.c
	 $(CC) $(CFLAGS) $^ -c

symtab.o: symtab.c
	 $(CC) $(CFLAGS) $^ -c

ahci: ahci.c lex.yy.c parser.o symtab.o
	 $(CC) $(CFLAGS) $^ -o $@ -lfl -lm -lreadline

test1: CFLAGS += -DAH_DEBUG 
test1: clean ahci
	@echo "a:4*(1-3);b:a*-1;b*b; ([x,y,z|x+y+z]@2,3,4)-8" | ./ahci

test2: CFLAGS += -DAH_DEBUG 
test2: clean ahci
	@echo "([a,b|a+b]@1,2)-1" | ./ahci

test3: CFLAGS += -DAH_DEBUG 
test3: clean ahci
	@echo "a:1; [x,y,z|x+y+z]{a,a+1,4} - 4" | ./ahci

test4: CFLAGS += -DAH_DEBUG 
test4: clean ahci
	@echo "a:7; ([n|a*n]@191)-1333" | ./ahci

test5: CFLAGS += -DAH_DEBUG 
test5: clean ahci
	@echo "116-(([x|[y|[z|x+y+z]]]@1)@10)@100" | ./ahci

test6: CFLAGS += -DAH_DEBUG 
test6: clean ahci
	@echo "([x|[y|if x<y [x] [y]]]@6)@7" | ./ahci

test7: CFLAGS += -DAH_DEBUG 
test7: clean ahci
	@echo "118 - (((([x|[f|[y|[z|f{x,y,z}]]]]@1)@[a,b,c|a+b+c])@10)@100)" | ./ahci

test8: CFLAGS += -DAH_DEBUG 
test8: clean ahci
	@echo "362888 - ([f | [x|x@x] @ [g | f@[args | (g@g)@args]]] @ [q | [w | if w<2 [1] [w*q{w-1}]]]) @ 9" | ./ahci

try: CFLAGS += -DAH_DEBUG 
try: clean ahci
	./ahci

debug: CFLAGS += -ggdb -DAH_DEBUG
debug: clean ahci
	gdb ahci

valgrind: CFLAGS += -ggdb -DAH_DEBUG
valgrind: clean ahci
	valgrind --leak-check=yes ./ahci

