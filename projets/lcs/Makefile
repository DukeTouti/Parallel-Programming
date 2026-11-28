# Makefile du Projet 1 - LCS : jeu de test qui lance les versions MPI et CUDA

CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L
TARGET = jeu_de_test

SOURCES = main.c jeu_de_test.c sequentiel.c
HEADERS = jeu_de_test.h sequentiel.h
OBJECTS = $(SOURCES:.c=.o)

# Regle par defaut : jeu de test + versions MPI et CUDA
all: $(TARGET)
	$(MAKE) -C MPI
	$(MAKE) -C CUDA

# Creation de l'executable (les .o sont supprimes aussitot)
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)
	rm -f $(OBJECTS)

# Compilation des fichiers objets
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Jeu de test complet : tailles croissantes, MPI et CUDA -> resultats.csv
run: all
	./$(TARGET)

# Graphes a partir de resultats.csv -> graphes/
graphes:
	python3 graphes.py

# Nettoyage des fichiers objets
clean:
	rm -f $(OBJECTS)
	$(MAKE) -C MPI clean
	$(MAKE) -C CUDA clean

# Nettoyage complet (executables, donnees generees, resultats, graphes)
fclean: clean
	rm -f $(TARGET) resultats.csv
	rm -rf donnees graphes
	$(MAKE) -C MPI fclean
	$(MAKE) -C CUDA fclean

# Recompilation complete
re: fclean all

.PHONY: all run graphes clean fclean re
