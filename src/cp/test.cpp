#include <benchmark/benchmark.h>



static void BM_Senumerate_initialize(benchmark::State& state) {
    const int N = 1e8;
    const int V = 11111;
    std::vector<int> arr(N,0);
    for (auto &x : arr) {
        x = V;
    }
}

BENCHMARK(BM_Senumerate_initialize);

static void BM_Smanual_initialize(benchmark::State& state) {
    const int N = 1e8;
    const int V = 11111;
    std::vector<int> arr(N,0);
    for(int i = 0 ; i < N ; ++i) {
        arr[i] = V;
    }
}

BENCHMARK(BM_Smanual_initialize);

BENCHMARK_MAIN();