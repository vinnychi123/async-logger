#include <benchmark/benchmark.h>
#include <thread>
#include <atomic>
#include "../include/logger.hpp"

// Benchmark 1: Single-threaded log call latency
static void BM_Logger_SingleLog(benchmark::State& state) {
    Logger logger("benchmark.log");
    int counter = 0;
    
    for (auto _ : state) {
        logger.log("Test message " + std::to_string(counter++));
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Logger_SingleLog);

// Benchmark 2: Log call latency with fixed string (no allocation)
static void BM_Logger_FixedString(benchmark::State& state) {
    Logger logger("benchmark.log");
    
    for (auto _ : state) {
        logger.log("Fixed test message");
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Logger_FixedString);

// Benchmark 3: Sustained throughput (how many logs/sec can we handle?)
static void BM_Logger_Throughput(benchmark::State& state) {
    Logger logger("benchmark.log");
    
    for (auto _ : state) {
        state.PauseTiming();
        constexpr int BATCH_SIZE = 10000;
        state.ResumeTiming();
        
        for (int i = 0; i < BATCH_SIZE; i++) {
            logger.log("Message " + std::to_string(i));
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_Logger_Throughput);

// Benchmark 4: Multi-threaded logging (realistic scenario)
static void BM_Logger_MultiThreaded(benchmark::State& state) {
    const int num_threads = state.range(0);
    constexpr int LOGS_PER_THREAD = 1000;
    Logger logger("benchmark_mt.log");
    
    for (auto _ : state) {
        std::atomic<bool> start{false};
        std::atomic<int> ready_count{0};
        
        std::vector<std::thread> threads;
        threads.reserve(num_threads);
        
        // Spawn threads
        for (int t = 0; t < num_threads; t++) {
            threads.emplace_back([&, t]() {
                ready_count++;
                while (!start.load(std::memory_order_acquire)) {}
                
                for (int i = 0; i < LOGS_PER_THREAD; i++) {
                    logger.log("Thread " + std::to_string(t) + " message " + std::to_string(i));
                }
            });
        }
        
        // Wait for all threads ready
        while (ready_count.load() < num_threads) {
            std::this_thread::yield();
        }
        
        // Start timing - all threads run
        start = true;
        
        for (auto& thread : threads) {
            thread.join();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_threads * LOGS_PER_THREAD);
}
BENCHMARK(BM_Logger_MultiThreaded)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

// Benchmark 5: Measure dropped logs under extreme load
static void BM_Logger_DropRate(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        Logger logger("benchmark_drop.log");
        state.ResumeTiming();
        
        // Flood the logger
        for (int i = 0; i < 100000; i++) {
            logger.log("Message " + std::to_string(i));
        }
        
        state.PauseTiming();
        uint64_t dropped = logger.get_dropped_count();
        state.counters["dropped"] = dropped;
        state.counters["drop_rate_%"] = (dropped * 100.0) / 100000.0;
        state.ResumeTiming();
    }
    
    state.SetItemsProcessed(state.iterations() * 100000);
}
BENCHMARK(BM_Logger_DropRate);

BENCHMARK_MAIN();