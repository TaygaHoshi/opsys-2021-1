build:
	gcc opsys.c -o opsys.o

run:
	./opsys.o

test:
	gcc opsys.c -o opsys.o
	./opsys.o
	rm opsys.o
