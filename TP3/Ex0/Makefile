NVCC      = nvcc
NVCCFLAGS = -O2

SRC = main.cu hello.cu
HDR = hello.cuh

all: bin/ex0_hello

bin/ex0_hello: $(SRC) $(HDR)
	@mkdir -p bin
	$(NVCC) $(NVCCFLAGS) $(SRC) -o $@

run: bin/ex0_hello
	bin/ex0_hello

clean:
	rm -rf bin

.PHONY: all run clean
