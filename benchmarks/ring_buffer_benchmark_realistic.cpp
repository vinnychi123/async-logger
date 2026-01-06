#include <benchmark/benchmark.h>
#include <thread>
#include <random>
#include "../include/ring_buffer.hpp"

// Prevent compiler from optimizing away operations
template<typename T>
void DoNotOptimize(T const& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}

// More realistic: varying data
static void BM_RingBuffer_Push_Realistic(benchmark::State& state) {
    RingBuffer<int, 1024> rb;
    int counter = 0;
    
    for (auto _ : state) {
        if (!rb.try_push(counter++)) {
            int dummy;
            while (rb.try_pop(dummy)) {}
            rb.try_push(counter);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RingBuffer_Push_Realistic);

// Measure actual SPSC throughput with separate threads
static void BM_RingBuffer_SPSC_Throughput(benchmark::State& state) {
    RingBuffer<int, 1024> rb;
    std::atomic<bool> stop{false};
    std::atomic<size_t> items_processed{0};
    
    for (auto _ : state) {
        state.PauseTiming();
        stop = false;
        items_processed = 0;
        
        // Consumer thread
        std::thread consumer([&]() {
            int value;
            while (!stop.load(std::memory_order_relaxed)) {
                if (rb.try_pop(value)) {
                    DoNotOptimize(value);
                    items_processed++;
                }
            }
            // Drain remaining
            while (rb.try_pop(value)) {
                DoNotOptimize(value);
                items_processed++;
            }
        });
        
        state.ResumeTiming();
        
        // Producer (main thread) - push for the iteration duration
        const size_t BATCH = 10000;
        for (size_t i = 0; i < BATCH; i++) {
            while (!rb.try_push(static_cast<int>(i))) {
                // Spin
            }
        }
        
        state.PauseTiming();
        stop = true;
        consumer.join();
        state.ResumeTiming();
    }
    
    state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_RingBuffer_SPSC_Throughput);

// Measure latency distribution (not just average)
static void BM_RingBuffer_Latency(benchmark::State& state) {
    RingBuffer<int, 1024> rb;
    std::vector<uint64_t> latencies;
    latencies.reserve(100000);
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Pre-fill to realistic level
        for (int i = 0; i < 256; i++) {
            rb.try_push(i);
        }
        
        state.ResumeTiming();
        
        // Measure push latency
        auto start = std::chrono::high_resolution_clock::now();
        rb.try_push(42);
        auto end = std::chrono::high_resolution_clock::now();
        
        latencies.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
        );
        
        // Pop to make space
        int dummy;
        rb.try_pop(dummy);
    }
    
    // Calculate percentiles
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        size_t p50_idx = latencies.size() / 2;
        size_t p99_idx = (latencies.size() * 99) / 100;
        size_t p999_idx = (latencies.size() * 999) / 1000;
        
        state.counters["p50_ns"] = latencies[p50_idx];
        state.counters["p99_ns"] = latencies[p99_idx];
        state.counters["p99.9_ns"] = latencies[p999_idx];
        state.counters["max_ns"] = latencies.back();
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RingBuffer_Latency)->Iterations(100000);

BENCHMARK_MAIN();