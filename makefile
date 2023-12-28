CFLAGS = -Wall

myshell: virtmem.c
	gcc $(CFLAGS) -o virtmem virtmem.c
	chmod +x virtmem
