# Makefile pour le Lab1 - Hands-on MPI

CC = mpicc
CFLAGS = -Wall -Wextra -g
NP = 4
# Nombre de coeurs de la machine, utilise par "make bench" pour tester
# le maximum de processus possible en une seule execution
NPMAX = $(shell nproc)
TARGET = main

SOURCES = main.c Hello.c PairImpair.c commun.c PingPong1.c PingPong2.c Ring.c MasterSlave.c SommeParallele.c
HEADERS = Hello.h PairImpair.h commun.h PingPong1.h PingPong2.h Ring.h MasterSlave.h SommeParallele.h
OBJECTS = $(SOURCES:.c=.o)

# Regle par defaut
all: $(TARGET)

# Creation de l'executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)
	rm -f $(OBJECTS)

# Compilation des fichiers objets
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage des fichiers objets
clean:
	rm -f $(OBJECTS)

# Nettoyage complet (objets + executable)
fclean: clean
	rm -f $(TARGET)

# Recompilation complete
re: fclean all

# Execution du programme avec NP processus (mpirun -np 4 ./main par defaut)
run: $(TARGET)
	mpirun -np $(NP) ./$(TARGET)

# Benchmark de l'exercice 5 : une seule execution avec NPMAX processus
# (= nproc, le nombre de coeurs de la machine), l'exercice 5 teste lui-meme
# tous les sous-comptes de processus (1, 2, 4, 8, 16...) via des sous-communicateurs.
# "0" est envoye sur l'entree standard pour l'exercice 4 qui demande une saisie.
bench: $(TARGET)
	@echo "Lancement avec NPMAX=$(NPMAX) processus (nproc de la machine)"
	echo "0" | mpirun -np $(NPMAX) --oversubscribe ./$(TARGET)

graphs:
	python3 plot_resultats.py

# Declaration des cibles qui ne sont pas des fichiers
.PHONY: all clean fclean re run bench graphs
