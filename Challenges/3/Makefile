NAME = rsa_text_mpi

CC = cc
MPICC = mpicc
CFLAGS = -Wall -Wextra

MPIRUN = mpirun --oversubscribe
NP ?= 4
ARGS ?=

# Paramètres du benchmark (voir bench.sh)
CHARS ?= 100000 1000000 5000000
REPS ?= 20
NPLIST ?= 1 2 4 8

SRC_RSA = rsa.c
SRC_TEXT = rsa_text_mpi.c
SRC_MAIN = main.c
OBJ_RSA = $(SRC_RSA:.c=.o)
OBJ_TEXT = $(SRC_TEXT:.c=.o)
OBJ_MAIN = $(SRC_MAIN:.c=.o)

all: $(NAME)

# Les .o sont générés puis supprimés aussitôt : seul l'exécutable reste
$(NAME): $(SRC_RSA) $(SRC_TEXT) $(SRC_MAIN) rsa.h rsa_text_mpi.h
	$(CC) $(CFLAGS) -c $(SRC_RSA) -o $(OBJ_RSA)
	$(MPICC) $(CFLAGS) -c $(SRC_TEXT) -o $(OBJ_TEXT)
	$(MPICC) $(CFLAGS) -c $(SRC_MAIN) -o $(OBJ_MAIN)
	$(MPICC) $(OBJ_MAIN) $(OBJ_TEXT) $(OBJ_RSA) -o $(NAME)
	rm -f $(OBJ_RSA) $(OBJ_TEXT) $(OBJ_MAIN)

# make run NP=8 pour changer le nombre de processus
# make run ARGS='"Bonjour le monde" alea' pour changer le message / activer l'alea
# make run NP=4 ARGS='@100000 std 10' : texte généré de 100000 caractères, 10 répétitions
run: $(NAME)
	$(MPIRUN) -np $(NP) ./$(NAME) $(ARGS)

# Exécution avec 2, 4 et 8 processus
run_all: $(NAME)
	@for n in 2 4 8; do \
		echo "===== np = $$n ====="; \
		$(MPIRUN) -np $$n ./$(NAME) $(ARGS); \
	done

# Benchmark : plusieurs tailles x plusieurs nombres de processus -> resultats.csv
# make bench CHARS="1000000 5000000" REPS=30 NPLIST="1 2 4"
bench: $(NAME)
	CHARS="$(CHARS)" REPS=$(REPS) NPLIST="$(NPLIST)" bash bench.sh

# Graphes à partir de resultats.csv -> graphes/
graphs:
	python3 plot_resultats.py

clean:
	rm -f $(OBJ_RSA) $(OBJ_TEXT) $(OBJ_MAIN)

fclean: clean
	rm -f $(NAME) resultats.csv
	rm -rf graphes

re: fclean all

.PHONY: all run run_all bench graphs clean fclean re
