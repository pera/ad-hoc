CFLAGS += -Wall -Wno-unused-variable -Wno-unused-function -std=gnu11

all: ahci

grammar: grammar.y
	 bison -Wall -rall -d grammar.y

scanner: scanner.l grammar
	 flex scanner.l

ahci: grammar scanner
	 $(CC) $(CFLAGS) parser.c grammar.tab.c lex.yy.c -o ahci -lfl -lm -lreadline

clean:
	rm -f grammar.tab.* grammar.output lex.yy.c ahci

test1: CFLAGS += -DAH_DEBUG 
test1: ahci
	@echo "a:4*(1-3);b:a*-1;b*b; ([x,y,z|x+y+z]@2,3,4)-8" | ./ahci

test2: CFLAGS += -DAH_DEBUG 
test2: ahci
	@echo "([a,b|a+b]@1,2)-1" | ./ahci

test3: CFLAGS += -DAH_DEBUG 
test3: ahci
	@echo "a:1; [x,y,z|x+y+z]{a,a+1,4} - 4" | ./ahci

test4: CFLAGS += -DAH_DEBUG 
test4: ahci
	@echo "a:7; ([n|a*n]@191)-1333" | ./ahci

test5: CFLAGS += -DAH_DEBUG 
test5: ahci
	@echo "116-(([x|[y|[z|x+y+z]]]@1)@10)@100" | ./ahci

test6: CFLAGS += -DAH_DEBUG 
test6: ahci
	@echo "([x|[y|if x<y [x] [y]]]@6)@7" | ./ahci

test7: CFLAGS += -DAH_DEBUG 
test7: ahci
	@echo "118 - (((([x|[f|[y|[z|f{x,y,z}]]]]@1)@[a,b,c|a+b+c])@10)@100)" | ./ahci

test8: CFLAGS += -DAH_DEBUG 
test8: ahci
	@echo "362888 - ([f | [x|x@x] @ [g | f@[args | (g@g)@args]]] @ [q | [w | if w<2 [1] [w*q{w-1}]]]) @ 9" | ./ahci

try: CFLAGS += -DAH_DEBUG 
try: ahci
	./ahci

debug: CFLAGS += -ggdb -DAH_DEBUG
debug: ahci
	gdb ahci

valgrind: CFLAGS += -ggdb -DAH_DEBUG
valgrind: ahci
	valgrind --leak-check=yes ./ahci

