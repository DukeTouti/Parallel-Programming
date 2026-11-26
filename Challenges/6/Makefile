CC        = gcc
CFLAGS    = -O2 -Wall -Wextra
LDLIBS    = -lm

MAXITER   ?= 1000000000
MAXPERIOD ?= 1000000000

.PHONY: all run plot clean

all: bench

bench: main.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

run: bench
	./bench $(MAXITER) $(MAXPERIOD)

plot:
	python3 plot.py

clean:
	rm -f bench resultats_fermat.csv resultats_shor.csv *.png
