pa2: pa2.c la2.h ipc.c
	gcc -std=c99 -Wall -pedantic *.c -o pa2 -L. -lruntime

clean:
	-rm -f *.o *.log pa2
