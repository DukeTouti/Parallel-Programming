NAME = rsa_mpi
BENCH = bench_mpi

CC = cc
MPICC = mpicc
CFLAGS = -Wall -Wextra

MPIRUN = mpirun --oversubscribe
NP ?= 4
NPMAX ?= 8
REP ?= 100000

SRC_RSA = rsa.c
SRC_MPI = rsa_mpi.c
SRC_BENCH = bench_mpi.c
OBJ_RSA = $(SRC_RSA:.c=.o)
OBJ_MPI = $(SRC_MPI:.c=.o)
OBJ_BENCH = $(SRC_BENCH:.c=.o)

all: $(NAME) $(BENCH)

# Les .o sont générés puis supprimés aussitôt : seuls les exécutables restent
$(NAME): $(SRC_RSA) $(SRC_MPI) rsa.h
	$(CC) $(CFLAGS) -c $(SRC_RSA) -o $(OBJ_RSA)
	$(MPICC) $(CFLAGS) -c $(SRC_MPI) -o $(OBJ_MPI)
	$(MPICC) $(OBJ_MPI) $(OBJ_RSA) -o $(NAME)
	rm -f $(OBJ_RSA) $(OBJ_MPI)

$(BENCH): $(SRC_RSA) $(SRC_BENCH) rsa.h
	$(CC) $(CFLAGS) -c $(SRC_RSA) -o $(OBJ_RSA)
	$(MPICC) $(CFLAGS) -c $(SRC_BENCH) -o $(OBJ_BENCH)
	$(MPICC) $(OBJ_BENCH) $(OBJ_RSA) -o $(BENCH)
	rm -f $(OBJ_RSA) $(OBJ_BENCH)

# make run NP=8 pour changer le nombre de processus
run: $(NAME)
	$(MPIRUN) -np $(NP) ./$(NAME)

# Exécution avec 1, 2, 4 et 8 processus
run_all: $(NAME)
	@for n in 1 2 4 8; do \
		echo "===== np = $$n ====="; \
		$(MPIRUN) -np $$n ./$(NAME); \
	done

# Benchmark : un seul lancement, les sous-communicateurs testent P = 1, 2, 4, 8
# make bench REP=500000 pour changer la charge
bench: $(BENCH)
	$(MPIRUN) -np $(NPMAX) ./$(BENCH) $(REP)

# Graphes à partir de resultats.csv
graphs:
	python3 plot_resultats.py

clean:
	rm -f $(OBJ_RSA) $(OBJ_MPI) $(OBJ_BENCH)

fclean: clean
	rm -f $(NAME) $(BENCH) resultats.csv
	rm -rf screens

re: fclean all

.PHONY: all run run_all bench graphs clean fclean re
