#include <iostream>
#include <cassert>
#include "../include/ring_buffer.hpp"

void test_basic_push_pop() {
    RingBuffer<int, 8> rb;
    
    // Test empty
    assert(rb.is_empty());
    assert(!rb.is_full());
    
    // Push one item
    assert(rb.try_push(42));
    assert(!rb.is_empty());
    
    // Pop it back
    int value;
    assert(rb.try_pop(value));
    assert(value == 42);
    assert(rb.is_empty());
    
    std::cout << "✓ test_basic_push_pop passed\n";
}

void test_fill_buffer() {
    RingBuffer<int, 8> rb;
    
    // Fill buffer (remember: capacity-1 items due to full/empty distinction)
    for (int i = 0; i < 7; i++) {
        assert(rb.try_push(i));
    }
    
    assert(rb.is_full());
    
    // Try to overfill - should fail
    assert(!rb.try_push(999));
    
    // Pop everything
    for (int i = 0; i < 7; i++) {
        int value;
        assert(rb.try_pop(value));
        assert(value == i);
    }
    
    assert(rb.is_empty());
    
    std::cout << "✓ test_fill_buffer passed\n";
}

void test_wraparound() {
    RingBuffer<int, 8> rb;
    
    // Push and pop more than capacity to test wraparound
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 7; i++) {
            assert(rb.try_push(i + round * 100));
        }
        
        for (int i = 0; i < 7; i++) {
            int value;
            assert(rb.try_pop(value));
            assert(value == i + round * 100);
        }
    }
    
    assert(rb.is_empty());
    
    std::cout << "✓ test_wraparound passed\n";
}

void test_pop_empty() {
    RingBuffer<int, 8> rb;
    
    int value;
    // Try to pop from empty buffer - should fail
    assert(!rb.try_pop(value));
    
    std::cout << "✓ test_pop_empty passed\n";
}

int main() {
    std::cout << "Running RingBuffer tests...\n\n";
    
    test_basic_push_pop();
    test_fill_buffer();
    test_wraparound();
    test_pop_empty();
    
    std::cout << "\n✅ All tests passed!\n";
    return 0;
}