#include <iostream>
#include <vector>
#include <algorithm>
#include "../include/ring_buffer.hpp"

#if defined(__aarch64__) || defined(__arm64__)
// ARM version - read system timer
inline uint64_t read_cycles() {
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}
#elif defined(__x86_64__) || defined(_M_X64)
// x86 version
inline uint64_t read_cycles() {
    unsigned int lo, hi;
    __asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}
#else
#error "Unsupported architecture"
#endif

int main() {
    RingBuffer<int, 1024> rb;
    constexpr size_t NUM_SAMPLES = 1000000;
    std::vector<uint64_t> latencies;
    latencies.reserve(NUM_SAMPLES);
    
    // Warmup
    for (int i = 0; i < 10000; i++) {
        rb.try_push(i);
        int dummy;
        rb.try_pop(dummy);
    }
    
    // Measure push latency
    for (size_t i = 0; i < NUM_SAMPLES; i++) {
        uint64_t start = read_cycles();
        rb.try_push(42);
        uint64_t end = read_cycles();
        
        latencies.push_back(end - start);
        
        // Pop to keep buffer from filling
        if (i % 100 == 0) {
            int dummy;
            for (int j = 0; j < 50; j++) rb.try_pop(dummy);
        }
    }
    
    // Calculate statistics
    std::sort(latencies.begin(), latencies.end());
    
    uint64_t min = latencies[0];
    uint64_t p50 = latencies[NUM_SAMPLES / 2];
    uint64_t p90 = latencies[(NUM_SAMPLES * 90) / 100];
    uint64_t p99 = latencies[(NUM_SAMPLES * 99) / 100];
    uint64_t p999 = latencies[(NUM_SAMPLES * 999) / 1000];
    uint64_t p9999 = latencies[(NUM_SAMPLES * 9999) / 10000];
    uint64_t max = latencies[NUM_SAMPLES - 1];
    
    // Calculate average
    uint64_t sum = 0;
    for (auto lat : latencies) sum += lat;
    double avg = static_cast<double>(sum) / NUM_SAMPLES;
    
    std::cout << "RingBuffer Push Latency (cycles):\n";
    std::cout << "==================================\n";
    std::cout << "Samples: " << NUM_SAMPLES << "\n";
    std::cout << "Min:     " << min << " cycles\n";
    std::cout << "Average: " << avg << " cycles\n";
    std::cout << "p50:     " << p50 << " cycles\n";
    std::cout << "p90:     " << p90 << " cycles\n";
    std::cout << "p99:     " << p99 << " cycles\n";
    std::cout << "p99.9:   " << p999 << " cycles\n";
    std::cout << "p99.99:  " << p9999 << " cycles\n";
    std::cout << "Max:     " << max << " cycles\n";
    
    std::cout << "\n(Note: ARM cycle counter frequency varies by chip)\n";
    
    return 0;
}