#include "ConvNetKerasBenchmarks.h"
#include "benchmark/benchmark.h"

static void BM_ConvolutionalNetwork_Keras_CPU(benchmark::State &state)
{
    TString architecture("CPU");

    // Benchmarking
    for (auto _ : state) {
        CNN_Keras_benchmark(architecture);
    }
}
BENCHMARK(BM_ConvolutionalNetwork_Keras_CPU);

BENCHMARK_MAIN();
