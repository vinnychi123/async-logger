#include <benchmark/benchmark.h>
#include <thread>
#include <atomic>
#include "../include/ring_buffer.hpp"

// Test current (optimized) version multiple times to get stable numbers
static void BM_CurrentSPSC_Run1(benchmark::State& state) {
    for (auto _ : state) {
        RingBuffer<int, 1024> rb;
        std::atomic<bool> stop{false};
        
        std::thread consumer([&]() {
            int value;
            while (!stop.load(std::memory_order_relaxed)) {
                rb.try_pop(value);
            }
            while (rb.try_pop(value)) {}
        });
        
        for (int i = 0; i < 10000; i++) {
            while (!rb.try_push(i)) {}
        }
        
        stop = true;
        consumer.join();
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_CurrentSPSC_Run1);

static void BM_CurrentSPSC_Run2(benchmark::State& state) {
    for (auto _ : state) {
        RingBuffer<int, 1024> rb;
        std::atomic<bool> stop{false};
        
        std::thread consumer([&]() {
            int value;
            while (!stop.load(std::memory_order_relaxed)) {
                rb.try_pop(value);
            }
            while (rb.try_pop(value)) {}
        });
        
        for (int i = 0; i < 10000; i++) {
            while (!rb.try_push(i)) {}
        }
        
        stop = true;
        consumer.join();
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_CurrentSPSC_Run2);

static void BM_CurrentSPSC_Run3(benchmark::State& state) {
    for (auto _ : state) {
        RingBuffer<int, 1024> rb;
        std::atomic<bool> stop{false};
        
        std::thread consumer([&]() {
            int value;
            while (!stop.load(std::memory_order_relaxed)) {
                rb.try_pop(value);
            }
            while (rb.try_pop(value)) {}
        });
        
        for (int i = 0; i < 10000; i++) {
            while (!rb.try_push(i)) {}
        }
        
        stop = true;
        consumer.join();
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_CurrentSPSC_Run3);

BENCHMARK_MAIN();