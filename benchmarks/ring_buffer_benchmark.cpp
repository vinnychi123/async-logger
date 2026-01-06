#include <benchmark/benchmark.h>
#include "../include/ring_buffer.hpp"

// Benchmark single push operation
static void BM_RingBuffer_Push(benchmark::State& state) {
    RingBuffer<int, 1024> rb;
    int value = 42;
    
    for (auto _ : state) {
        if (!rb.try_push(value)) {
            // Reset if full
            int dummy;
            rb.try_pop(dummy);
            rb.try_push(value);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RingBuffer_Push);

// Benchmark single pop operation
static void BM_RingBuffer_Pop(benchmark::State& state) {
    RingBuffer<int, 1024> rb;
    
    // Pre-fill buffer
    for (int i = 0; i < 512; i++) {
        rb.try_push(i);
    }
    
    int value;
    for (auto _ : state) {
        if (!rb.try_pop(value)) {
            // Refill if empty
            rb.try_push(42);
            rb.try_pop(value);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RingBuffer_Pop);

// Benchmark push/pop pair (full round-trip)
static void BM_RingBuffer_PushPop(benchmark::State& state) {
    RingBuffer<int, 1024> rb;
    int value = 42;
    int result;
    
    for (auto _ : state) {
        rb.try_push(value);
        rb.try_pop(result);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RingBuffer_PushPop);

// Benchmark with different buffer sizes
static void BM_RingBuffer_Push_Size(benchmark::State& state) {
    const size_t size = state.range(0);
    int value = 42;
    
    if (size == 64) {
        RingBuffer<int, 64> rb;
        for (auto _ : state) {
            int dummy;
            if (!rb.try_push(value)) { rb.try_pop(dummy); rb.try_push(value); }
        }
    } else if (size == 256) {
        RingBuffer<int, 256> rb;
        for (auto _ : state) {
            int dummy;
            if (!rb.try_push(value)) { rb.try_pop(dummy); rb.try_push(value); }
        }
    } else if (size == 1024) {
        RingBuffer<int, 1024> rb;
        for (auto _ : state) {
            int dummy;
            if (!rb.try_push(value)) { rb.try_pop(dummy); rb.try_push(value); }
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RingBuffer_Push_Size)->Arg(64)->Arg(256)->Arg(1024);

BENCHMARK_MAIN();