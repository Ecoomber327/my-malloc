my-malloc.so: my-malloc.c
	gcc -Wall -pedantic -rdynamic -shared -fPIC -g -o my-malloc.so my-malloc.c

test: test.c
	gcc -Wall -pedantic -g -o test test.c

.PHONY: clean
clean:
	rm -f test my-malloc.so

.PHONY: runtests
runtests: my-malloc.so test
	LD_PRELOAD=./my-malloc.so ./test

.PHONY: gdb-help
gdb-help: my-malloc.so test
	gdb --args env LD_PRELOAD=./my-malloc.so ./test


.PHONY: testls
testls: my-malloc.so
		LD_PRELOAD=./my-malloc.so ls

.PHONY: testls-l
testls-l: my-malloc.so
		LD_PRELOAD=./my-malloc.so ls -l

.PHONY: testls-R
testls-R: my-malloc.so
		LD_PRELOAD=./my-malloc.so ls -R

.PHONY: gdbls-l
gdbls-l: my-malloc.so
	gdb --args env LD_PRELOAD=./my-malloc.so ls -l

.PHONY: gdbls
gdbls: my-malloc.so
	gdb --args env LD_PRELOAD=./my-malloc.so ls
