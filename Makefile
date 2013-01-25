CC=gcc
CFLAGS=-lm

all : com_sheet
debug : CFLAGS=-lm -ggdb
debug : com_sheet

com_sheet : uni_records.o

uni_records.o : uni_records.c formulae.c formulae.h color_vars.h
		$(CC) $(CFLAGS) uni_records.c formulae.c -o com_sheet

clean :
	rm com_sheet uni_records.o formulae.o
