from math import gcd
from time import perf_counter


# ==========================================================
# Classical Shor-Inspired Factorization Algorithm
# ==========================================================

def classical_shor_factor(N, a, max_period=10_000_000):

    if N <= 1:
        raise ValueError("N must be greater than 1")

    if not (1 < a < N):
        raise ValueError("a must satisfy 1 < a < N")

    # Step 1: Calculate gcd(a, N)
    d = gcd(a, N)

    # A non-trivial gcd already gives a factor
    if 1 < d < N:
        return d, N // d, 0, 0

    if d != 1:
        return None, None, None, 0

    # Step 2: Search for the period
    # Find the smallest r such that a^r mod N = 1

    value = 1
    iterations = 0
    r = None

    for k in range(1, max_period + 1):

        value = (value * a) % N
        iterations += 1

        if value == 1:
            r = k
            break

    # Period not found within the limit
    if r is None:
        return None, None, None, iterations

    # Step 3: The period must be even
    if r % 2 != 0:
        return None, None, r, iterations

    # Step 4: Calculate a^(r/2) mod N
    x = pow(a, r // 2, N)

    # Step 5: Avoid the trivial case
    if x == N - 1:
        return None, None, r, iterations

    # Step 6: Extract factors using GCD
    p = gcd(x - 1, N)
    q = gcd(x + 1, N)

    # Step 7: Verify the factorization
    if 1 < p < N and 1 < q < N and p * q == N:
        return p, q, r, iterations

    return None, None, r, iterations


# ==========================================================
# Experiment 1: Increasing N
# ==========================================================

# Each number is a product of the two displayed factors.
# The selected base is a = 2.

test_cases = [

    (10403, 2),                  # 101 * 103

    (1022117, 2),                # 1009 * 1013

    (100440259, 2),              # 10007 * 10037

    (10002200057, 2),            # 100003 * 100019

    (1000036000099, 2),          # 1000003 * 1000033

    (15000550003, 2),            # 100003 * 150001

    (20000900009, 2),            # 100003 * 200003

    (50002400027, 2),            # 100003 * 500009

    (100003300009, 2)            # 100003 * 1000003
]


# Maximum number of iterations for period search
MAX_PERIOD = 100_000_000


# ==========================================================
# Performance Evaluation
# ==========================================================

print("=" * 115)
print("CLASSICAL SHOR-INSPIRED FACTORIZATION - PERFORMANCE EVALUATION")
print("=" * 115)

print("{:<17} {:<8} {:<13} {:<13} {:<13} {:<15} {:<15}".format(
    "N", "Base", "Factor p", "Factor q",
    "Period r", "Iterations", "Time (seconds)"
))

print("-" * 115)


for N, a in test_cases:

    start_time = perf_counter()

    p, q, r, iterations = classical_shor_factor(
        N, a, MAX_PERIOD
    )

    execution_time = perf_counter() - start_time

    print("{:<17} {:<8} {:<13} {:<13} {:<13} {:<15} {:.8f}".format(
        N,
        a,
        str(p),
        str(q),
        str(r),
        iterations,
        execution_time
    ))

    if p is not None:
        print("   SUCCESS:", N, "=", p, "*", q)

    elif r is None:
        print("   Period not found within the iteration limit.")

    else:
        print("   Period found, but no non-trivial factorization obtained.")


print("=" * 115)


# ==========================================================
# Experiment 2: Influence of the Base a
# ==========================================================

# Keep N fixed and change the base a.

N = 10403

bases = [2, 3, 5, 7, 11, 13, 17, 19, 23]

print("\n")
print("=" * 105)
print("INFLUENCE OF THE BASE a ON CLASSICAL PERIOD FINDING")
print("=" * 105)

print("N =", N)

print("{:<10} {:<13} {:<13} {:<13} {:<15} {:<15}".format(
    "Base a", "Factor p", "Factor q",
    "Period r", "Iterations", "Time (seconds)"
))

print("-" * 105)


for a in bases:

    start_time = perf_counter()

    p, q, r, iterations = classical_shor_factor(
        N, a, MAX_PERIOD
    )

    execution_time = perf_counter() - start_time

    print("{:<10} {:<13} {:<13} {:<13} {:<15} {:.8f}".format(
        a,
        str(p),
        str(q),
        str(r),
        iterations,
        execution_time
    ))

    if p is not None:
        print("   SUCCESS:", N, "=", p, "*", q)

    elif r is None:
        print("   Period not found.")

    else:
        print("   No non-trivial factorization for this base.")


print("=" * 105)
