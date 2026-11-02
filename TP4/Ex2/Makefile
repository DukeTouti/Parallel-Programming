NVCC      = nvcc
NVCCFLAGS = -O2

T_BLOCK = 512
CSV     = resultats.csv

SRC = main.cu somme.cu benchmark.cu
HDR = somme.cuh benchmark.cuh

# Tailles divisibles par T_BLOCK ; <=1024 permettent aussi <<<1,N>>>
TAILLES = 512 1024 65536 262144 1048576 4194304 16777216

all: bin/ex2_sum

bin/ex2_sum: $(SRC) $(HDR)
	@mkdir -p bin
	$(NVCC) $(NVCCFLAGS) -DT_BLOCK=$(T_BLOCK) $(SRC) -o $@

# version simple (equivalent de vecadd.cu) : bin/sum_simple [N] [T]
bin/sum_simple: sum.cu
	@mkdir -p bin
	$(NVCC) $(NVCCFLAGS) sum.cu -o $@

run: bin/ex2_sum
	bin/ex2_sum

bench: bin/ex2_sum
	rm -f $(CSV)
	for n in $(TAILLES); do bin/ex2_sum $$n $(CSV); done

graphes: $(CSV)
	python3 graphes.py $(CSV) graphes

clean:
	rm -rf bin

fclean: clean
	rm -rf $(CSV) graphes

.PHONY: all run bench graphes clean fclean
