# Makefile du Projet 1 - LCS (version MPI)

CC = mpicc
CFLAGS = -O2 -Wall -Wextra
MPIRUN = mpirun --oversubscribe
NP = 4
TARGET = lcs_mpi

SOURCES = main.c lcs_mpi.c sequences.c
HEADERS = lcs_mpi.h sequences.h
OBJECTS = $(SOURCES:.c=.o)

# Regle par defaut
all: $(TARGET)

# Creation de l'executable (les .o sont supprimes aussitot)
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)
	rm -f $(OBJECTS)

# Compilation des fichiers objets
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Execution seule sur des sequences aleatoires (make run NP=8 M=8000 N=8000)
M = 2000
N = 2000
run: $(TARGET)
	$(MPIRUN) -np $(NP) ./$(TARGET) $(M) $(N)

# Nettoyage des fichiers objets
clean:
	rm -f $(OBJECTS)

# Nettoyage complet
fclean: clean
	rm -rf $(TARGET)

# Recompilation complete
re: fclean all

.PHONY: all run clean fclean re
