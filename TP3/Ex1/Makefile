NVCC      = nvcc
NVCCFLAGS = -O2

T_BLOCK = 512
CSV     = resultats.csv

SRC = main.cu vecteur.cu benchmark.cu
HDR = vecteur.cuh benchmark.cuh

# Tailles divisibles par T_BLOCK ; <=1024 permettent aussi <<<1,N>>>
TAILLES = 512 1024 65536 262144 720896 1048576 4194304

all: bin/ex1_vecadd

bin/ex1_vecadd: $(SRC) $(HDR)
	@mkdir -p bin
	$(NVCC) $(NVCCFLAGS) -DT_BLOCK=$(T_BLOCK) $(SRC) -o $@

run: bin/ex1_vecadd
	bin/ex1_vecadd

bench: bin/ex1_vecadd
	rm -f $(CSV)
	for n in $(TAILLES); do bin/ex1_vecadd $$n $(CSV); done

graphes: $(CSV)
	python3 graphes.py $(CSV) graphes

clean:
	rm -rf bin

fclean: clean
	rm -rf $(CSV) graphes

.PHONY: all run bench graphes clean fclean
