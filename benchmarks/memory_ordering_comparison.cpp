#include <benchmark/benchmark.h>
#include <iostream>

// You'll need two versions of RingBuffer to compare
// For now, let's just benchmark the new version

#include "../include/ring_buffer.hpp"
#include <thread>

static void BM_OptimizedPush(benchmark::State& state) {
    RingBuffer<int, 1024> rb;
    int value = 42;
    
    for (auto _ : state) {
        if (!rb.try_push(value)) {
            int dummy;
            rb.try_pop(dummy);
            rb.try_push(value);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OptimizedPush);

static void BM_OptimizedPushPop(benchmark::State& state) {
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
BENCHMARK(BM_OptimizedPushPop);

static void BM_OptimizedSPSC(benchmark::State& state) {
    RingBuffer<int, 1024> rb;
    std::atomic<bool> stop{false};
    
    for (auto _ : state) {
        state.PauseTiming();
        stop = false;
        
        std::thread consumer([&]() {
            int value;
            while (!stop.load(std::memory_order_relaxed)) {
                if (rb.try_pop(value)) {
                    benchmark::DoNotOptimize(value);
                }
            }
            while (rb.try_pop(value)) {
                benchmark::DoNotOptimize(value);
            }
        });
        
        state.ResumeTiming();
        
        for (int i = 0; i < 10000; i++) {
            while (!rb.try_push(i)) {}
        }
        
        state.PauseTiming();
        stop = true;
        consumer.join();
        state.ResumeTiming();
    }
    
    state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_OptimizedSPSC);

BENCHMARK_MAIN();