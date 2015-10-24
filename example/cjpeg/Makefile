# Makefile2
cjpeg: cjpeg_main.o cjpeg_proc.o
	gcc -Wall -O2 -o cjpeg cjpeg_main.o cjpeg_proc.o

cjpeg_proc.o: cjpeg_proc.c
	gcc -c cjpeg_proc.c

cjpeg_main.o: cjpeg_main.c
	gcc -c cjpeg_main.c
