#include <benchmark/benchmark.h>

// static void BM_StringCreation(benchmark::State& state) {
//   for (auto _ : state)
//     std::string empty_string;
// }
// // Register the function as a benchmark
// BENCHMARK(BM_StringCreation);

// // Define another benchmark
// static void BM_StringCopy(benchmark::State& state) {
//   std::string x = "hello";
//   for (auto _ : state)
//     std::string copy(x);
// }
// BENCHMARK(BM_StringCopy);

// BENCHMARK_MAIN();


// #include <numeric>
// #include <algorithm>

// static void BM_StdGcd(benchmark::State& state) {
//     int a = state.range(0);
//     int b = state.range(1);
//     for (auto _ : state) {
//         volatile int result = std::gcd(a, b);
//         benchmark::DoNotOptimize(result);
//     }
// }

// static void BM_GnuGcd(benchmark::State& state) {
//     int a = state.range(0);
//     int b = state.range(1);
//     for (auto _ : state) {
//         volatile int result = std::__gcd(a, b);
//         benchmark::DoNotOptimize(result);
//     }
// }

// // Register with different input values
// BENCHMARK(BM_StdGcd)->Args({10, 6})->Args({100, 75})->Args({123456, 7890});
// BENCHMARK(BM_GnuGcd)->Args({10, 6})->Args({100, 75})->Args({123456, 7890});

// BENCHMARK_MAIN();

static void BM_IntegerAddition(benchmark::State& state) {
    int a = state.range(0);
    int b = state.range(1);
    for (auto _ : state) {
        volatile int result = a + b;
        // benchmark::DoNotOptimize(result);
    }
}
// Register the function as a benchmark

// Define another benchmark
static void BM_FloatAddition(benchmark::State& state) {
    float a = state.range(0);
    float b = state.range(1);
    for(auto _ : state) {
        volatile float result = a + b;
        // benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_IntegerAddition)->Args({1, 2});
BENCHMARK(BM_FloatAddition)->Args({1, 2});

BENCHMARK_MAIN();

// g++ mybenchmark.cc -std=c++11 -isystem benchmark/include -Lbenchmark/build/src -lbenchmark -lpthread -o mybenchmark