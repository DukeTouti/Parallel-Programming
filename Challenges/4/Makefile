NVCC      = nvcc
CFLAGS    = -O2 -arch=sm_75
LDLIBS    = -lm

MAXTESTS ?= 50000000
TPB      ?= 256 512
BLOCKS   ?= 256 512 1024
OUT       = resultats.csv

.PHONY: all run plot clean

all: bench fermat

# Programme principal du TP
fermat: fermat.cu
	$(NVCC) $(CFLAGS) $< -o $@ $(LDLIBS)

# Benchmark (main.cu)
bench: main.cu
	$(NVCC) $(CFLAGS) $< -o $@ $(LDLIBS)

# Lance le benchmark pour chaque combinaison blocks x threads
run: bench
	echo "test,N,tpb,blocks,cpu_s,gpu_s,speedup,ok" > $(OUT)
	for tpb in $(TPB); do \
		for blk in $(BLOCKS); do \
			echo "=== tpb=$$tpb  blocks=$$blk ==="; \
			./bench $(OUT) $(MAXTESTS) $$tpb $$blk || exit 1; \
		done; \
	done
	@echo "Résultats dans $(OUT)"

plot:
	python3 plot.py $(OUT)

clean:
	rm -f bench fermat $(OUT) *.png
