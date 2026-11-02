NVCC      = nvcc
NVCCFLAGS = -O2

T_BLOCK = 128
CSV     = resultats.csv

SRC = main.cu matrice.cu benchmark.cu
HDR = matrice.cuh benchmark.cuh

# Tailles divisibles par T_BLOCK ; <=1024 permettent aussi <<<1,N>>>
# (4096 possible mais la reference CPU dure plusieurs minutes)
TAILLES = 128 256 512 1024 2048

all: bin/ex3_mmult

bin/ex3_mmult: $(SRC) $(HDR)
	@mkdir -p bin
	$(NVCC) $(NVCCFLAGS) -DT_BLOCK=$(T_BLOCK) $(SRC) -o $@

# version simple (template mmult.cu complete) : bin/mmult_simple [T] [N]
bin/mmult_simple: mmult.cu
	@mkdir -p bin
	$(NVCC) $(NVCCFLAGS) mmult.cu -o $@

run: bin/ex3_mmult
	bin/ex3_mmult

bench: bin/ex3_mmult
	rm -f $(CSV)
	for n in $(TAILLES); do bin/ex3_mmult $$n $(CSV); done

graphes: $(CSV)
	python3 graphes.py $(CSV) graphes

clean:
	rm -rf bin

fclean: clean
	rm -rf $(CSV) graphes

.PHONY: all run bench graphes clean fclean
