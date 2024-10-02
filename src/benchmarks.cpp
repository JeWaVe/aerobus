#include "./aerobus.h"
#include <benchmark/benchmark.h>  // NOLINT
#include <cmath>  // NOLINT

static constexpr size_t N = 1 << 23;

double rand(double min, double max) {
  double range = (max - min);
  double div = RAND_MAX / range;
  return min + (rand() / div);  // NOLINT
}

INLINED double aero_expm1_12(const double x) {
    using EXPM1 = aerobus::expm1<aerobus::i64, 13>;
    return EXPM1::eval(EXPM1::eval(EXPM1::eval(EXPM1::eval(EXPM1::eval(EXPM1::eval(
                EXPM1::eval(EXPM1::eval(EXPM1::eval(EXPM1::eval(EXPM1::eval(EXPM1::eval(x))))))))))));
}

static void BM_aero_expm1_12(benchmark::State &state) {
    double *in = aerobus::aligned_malloc<double>(N, 64);
    double *out = aerobus::aligned_malloc<double>(N, 64);
    #pragma omp parallel for
    for (size_t i = 0; i < N; ++i) {
        in[i] = rand(-0.01, 0.01);
    }
    for (auto _ : state) {
        #pragma omp parallel for
        for (size_t i = 0; i < N; ++i) {
            out[i] = aero_expm1_12(in[i]);
        }
    }
}

static void BM_std_expm1_12(benchmark::State &state) {
    double *in = aerobus::aligned_malloc<double>(N, 64);
    double *out = aerobus::aligned_malloc<double>(N, 64);
    #pragma omp parallel for
    for (size_t i = 0; i < N; ++i) {
        in[i] = rand(-0.01, 0.01);
    }
    for (auto _ : state) {
        #pragma omp parallel for
        for (size_t i = 0; i < N; ++i) {
            out[i] = ::expm1(::expm1(::expm1(::expm1(::expm1(::expm1(
                ::expm1(::expm1(::expm1(::expm1(::expm1(::expm1(in[i]))))))))))));
        }
    }
}

static void BM_aero_hermite(benchmark::State &state) {
    double *in = aerobus::aligned_malloc<double>(N, 64);
    double *out = aerobus::aligned_malloc<double>(N, 64);
    #pragma omp parallel for
    for (size_t i = 0; i < N; ++i) {
        in[i] = rand(-0.01, 0.01);
    }
    for (auto _ : state) {
        #pragma omp parallel for
        for (size_t i = 0; i < N; ++i) {
            out[i] = aerobus::known_polynomials::hermite_phys<12>::eval<double>(in[i]);
        }
    }
}

static void BM_std_hermite(benchmark::State &state) {
    double *in = aerobus::aligned_malloc<double>(N, 64);
    double *out = aerobus::aligned_malloc<double>(N, 64);
    #pragma omp parallel for
    for (size_t i = 0; i < N; ++i) {
        in[i] = rand(-0.01, 0.01);
    }
    for (auto _ : state) {
        #pragma omp parallel for
        for (size_t i = 0; i < N; ++i) {
            out[i] = std::hermite(12, in[i]);
        }
    }
}

BENCHMARK(BM_std_expm1_12);
BENCHMARK(BM_aero_expm1_12);
BENCHMARK(BM_std_hermite);
BENCHMARK(BM_aero_hermite);

BENCHMARK_MAIN();
