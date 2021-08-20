#include "RNNKerasBenchmarks.h"
#include "benchmark/benchmark.h"

static void BM_RNN_CPU_Keras(benchmark::State &state)
{
    TString architecture("CPU");
    
    // Benchmarking
    for (auto _ : state) {
        RNN_Keras_benchmark(architecture);
    }
}
BENCHMARK(BM_RNN_CPU_Keras);

BENCHMARK_MAIN();
