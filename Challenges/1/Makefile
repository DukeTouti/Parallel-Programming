CC       = mpicc
CFLAGS   = -O2 -Wall
LDLIBS   = -lm

MAXTESTS ?= 50000000
FULL     ?= 1
NPS      ?= 1 2 4 8
OUT      = resultats.csv

.PHONY: all run plot clean

all: bench

# Benchmark (main.c)
bench: main.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

# Programme du TP (fermat.c complété)
fermat: fermat.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

# Lance le benchmark pour chaque nombre de processus -> resultats.csv
# Exemples : make run
#            make run NPS="1 2 4" MAXTESTS=20000000 FULL=0
run: bench
	echo "test,N,np,maxTests,temps_s,ok" > $(OUT)
	for np in $(NPS); do \
		echo "=== np = $$np ==="; \
		mpirun --oversubscribe -np $$np ./bench $(OUT) $(MAXTESTS) $(FULL) || exit 1; \
	done
	@echo "Résultats dans $(OUT)"

# Trace les courbes depuis le CSV
plot:
	python3 plot.py $(OUT)

clean:
	rm -f bench fermat $(OUT) temps_vs_N.png speedup_vs_N.png speedup_vs_np.png
