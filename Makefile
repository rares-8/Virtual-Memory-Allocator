CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g
build: vma

vma: main.c vma.c
	$(CC) vma.c main.c -o vma $(CFLAGS)

run_vma:
	./vma

clean:
	rm -f vma