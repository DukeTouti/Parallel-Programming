
from math import isqrt
from time import perf_counter


# ==========================================================
# Fermat Factorization Algorithm
# ==========================================================

def fermat_factor(N):

    if N <= 1:
        raise ValueError("N must be greater than 1")

    if N % 2 == 0:
        return 2, N // 2, 0

    x = isqrt(N)

    if x * x < N:
        x += 1

    iterations = 0

    while True:

        iterations += 1

        y_squared = x * x - N
        y = isqrt(y_squared)

        if y * y == y_squared:

            p = x - y
            q = x + y

            if p * q == N:
                return p, q, iterations

        x += 1


# ==========================================================
# Test Numbers
# ==========================================================

test_numbers = [

    10403,                  # 101 * 103

    1012027,                # 1003 * 1009

    100440259,              # 10007 * 10037

    10002200057,            # 100003 * 100019

    1000036000099,          # 1000003 * 1000033

    11002630051,            # 100003 * 110017

    15000750003,            # 100003 * 150001

    2000090009,             # 100003 * 200003

    50002000027,            # 100003 * 500009

    100003300009             # 100003 * 1000003
]


# ==========================================================
# Performance Evaluation
# ==========================================================

print("=" * 75)
print("FERMAT FACTORIZATION - PERFORMANCE EVALUATION")
print("=" * 75)

print("{:<18} {:<12} {:<12} {:<15} {:<15}".format(
    "N", "Factor p", "Factor q", "Iterations", "Time (seconds)"
))

print("-" * 75)


for N in test_numbers:

    start_time = perf_counter()

    p, q, iterations = fermat_factor(N)

    end_time = perf_counter()

    execution_time = end_time - start_time

    print("{:<18} {:<12} {:<12} {:<15} {:.8f}".format(
        N, p, q, iterations, execution_time
    ))


print("=" * 75)

