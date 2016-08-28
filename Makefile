CC   =	gcc
COPT =	-g -w -m32

all: whitney

whitney:
	$(CC) $(COPT) src/whitney.c -o bin/whitney

clean:
	rm -rf bin/whitney whitney.dSYM
