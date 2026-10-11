CC = mpicc
CFLAGS = -O2 -Wall -Wextra -std=c11 -fwrapv

SRC = main.c benchmark.c vectadd.c matvec.c matmul.c outils.c
HDR = benchmark.h vectadd.h matvec.h matmul.h outils.h

# Adapter a ton nombre de coeurs physiques (puissances de 2 : SIZE divisible par P)
NP_LISTE = 1 2 4 8
TAILLES_VECT = 1000 10000 100000 1000000 10000000
TAILLES_MATVEC = 256 512 1024 2048 4096
TAILLES_MATMAT = 64 128 256 512 1024

MPIRUN = mpirun --oversubscribe
CSV = resultats.csv

EXECS_VECT = $(addprefix bin/tp2_vect_,$(TAILLES_VECT))
EXECS_MAT = $(addprefix bin/tp2_mat_,$(sort $(TAILLES_MATVEC) $(TAILLES_MATMAT)))

all: bin/tp2_test

# Un executable par taille (SIZE / ARRAY_SIZE sont des constantes de compilation)
bin/tp2_vect_%: $(SRC) $(HDR)
	@mkdir -p bin
	$(CC) $(CFLAGS) -DARRAY_SIZE=$* $(SRC) -o $@

bin/tp2_mat_%: $(SRC) $(HDR)
	@mkdir -p bin
	$(CC) $(CFLAGS) -DSIZE=$* $(SRC) -o $@

# Tailles par defaut du prof (8) : affichage des matrices et verification
bin/tp2_test: $(SRC) $(HDR)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $@

test: bin/tp2_test
	for np in 1 2 4 8; do $(MPIRUN) -np $$np bin/tp2_test test || exit 1; done

bench: $(EXECS_VECT) $(EXECS_MAT)
	rm -f $(CSV)
	for np in $(NP_LISTE); do \
		for n in $(TAILLES_VECT); do $(MPIRUN) -np $$np bin/tp2_vect_$$n vecteur $(CSV) || exit 1; done; \
		for n in $(TAILLES_MATVEC); do $(MPIRUN) -np $$np bin/tp2_mat_$$n matvec $(CSV) || exit 1; done; \
		for n in $(TAILLES_MATMAT); do $(MPIRUN) -np $$np bin/tp2_mat_$$n matmat $(CSV) || exit 1; done; \
	done

graphes: $(CSV)
	python3 graphes.py $(CSV) graphes

clean:
	rm -rf bin

fclean: clean
	rm -rf $(CSV) graphes

.PHONY: all test bench graphes clean fclean
