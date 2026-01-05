#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <cassert>
#include "../include/ring_buffer.hpp"

// Test: Producer pushes N items, consumer pops N items
void test_spsc_correctness() {
    constexpr size_t NUM_ITEMS = 1000000;
    RingBuffer<int, 1024> rb;
    
    std::atomic<bool> producer_done{false};
    std::atomic<size_t> items_pushed{0};
    std::atomic<size_t> items_popped{0};
    
    // Producer thread
    std::thread producer([&]() {
        for (size_t i = 0; i < NUM_ITEMS; i++) {
            while (!rb.try_push(static_cast<int>(i))) {
                // Spin until space available
                std::this_thread::yield();
            }
            items_pushed++;
        }
        producer_done = true;
    });
    
    // Consumer thread
    std::thread consumer([&]() {
        std::vector<int> received;
        received.reserve(NUM_ITEMS);
        
        while (received.size() < NUM_ITEMS) {
            int value;
            if (rb.try_pop(value)) {
                received.push_back(value);
                items_popped++;
            } else {
                std::this_thread::yield();
            }
        }
        
        // Verify all values received in order
        for (size_t i = 0; i < NUM_ITEMS; i++) {
            assert(received[i] == static_cast<int>(i));
        }
    });
    
    producer.join();
    consumer.join();
    
    assert(items_pushed == NUM_ITEMS);
    assert(items_popped == NUM_ITEMS);
    assert(rb.is_empty());
    
    std::cout << "✓ test_spsc_correctness passed (" << NUM_ITEMS << " items)\n";
}

// Test: High contention - small buffer, many items
void test_high_contention() {
    constexpr size_t NUM_ITEMS = 100000;
    RingBuffer<int, 16> rb;  // Small buffer = more contention
    
    std::atomic<size_t> sum_pushed{0};
    std::atomic<size_t> sum_popped{0};
    
    std::thread producer([&]() {
        for (size_t i = 0; i < NUM_ITEMS; i++) {
            while (!rb.try_push(static_cast<int>(i))) {
                std::this_thread::yield();
            }
            sum_pushed += i;
        }
    });
    
    std::thread consumer([&]() {
        size_t count = 0;
        while (count < NUM_ITEMS) {
            int value;
            if (rb.try_pop(value)) {
                sum_popped += value;
                count++;
            } else {
                std::this_thread::yield();
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    // Verify sum matches (no data corruption)
    assert(sum_pushed == sum_popped);
    assert(rb.is_empty());
    
    std::cout << "✓ test_high_contention passed (" << NUM_ITEMS << " items)\n";
}

// Test: Burst workload
void test_burst_workload() {
    constexpr size_t NUM_BURSTS = 1000;
    constexpr size_t BURST_SIZE = 100;
    RingBuffer<int, 256> rb;
    
    std::atomic<size_t> total_items{0};
    
    std::thread producer([&]() {
        for (size_t burst = 0; burst < NUM_BURSTS; burst++) {
            // Push burst
            for (size_t i = 0; i < BURST_SIZE; i++) {
                while (!rb.try_push(static_cast<int>(i))) {
                    std::this_thread::yield();
                }
            }
            // Small delay between bursts
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });
    
    std::thread consumer([&]() {
        size_t expected = NUM_BURSTS * BURST_SIZE;
        while (total_items < expected) {
            int value;
            if (rb.try_pop(value)) {
                total_items++;
            } else {
                std::this_thread::yield();
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    assert(total_items == NUM_BURSTS * BURST_SIZE);
    assert(rb.is_empty());
    
    std::cout << "✓ test_burst_workload passed\n";
}

int main() {
    std::cout << "Running multi-threaded RingBuffer tests...\n\n";
    
    test_spsc_correctness();
    test_high_contention();
    test_burst_workload();
    
    std::cout << "\n✅ All multi-threaded tests passed!\n";
    std::cout << "Your SPSC RingBuffer is working correctly under concurrent access.\n";
    
    return 0;
}