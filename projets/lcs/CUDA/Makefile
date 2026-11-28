# Makefile du Projet 1 - LCS (version CUDA)

NVCC = nvcc
# native : compile pour le GPU de la machine (RTX 3060 Laptop = sm_86)
ARCH = native
NVCCFLAGS = -O2 -arch=$(ARCH)
TARGET = lcs_cuda

SRC = main.cu lcs_cuda.cu sequences.cu
HDR = lcs_cuda.cuh sequences.cuh

# Regle par defaut
all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(NVCC) $(NVCCFLAGS) $(SRC) -o $@

# Execution seule sur des sequences aleatoires (make run M=8000 N=8000 METHODE=diagonales)
M = 2000
N = 2000
METHODE = tuiles
run: $(TARGET)
	./$(TARGET) $(M) $(N) $(METHODE)

clean:
	rm -rf $(TARGET)

fclean: clean

re: fclean all

.PHONY: all run clean fclean re
