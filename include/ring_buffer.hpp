#pragma once
#include <atomic>
#include <cstddef>


template <typename T, std::size_t Capacity>
class RingBuffer{
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

    public:
        RingBuffer(): head_(0), tail_(0) {}
        
        bool try_push(const T& item) {
            size_t cur_head = head_.load(std::memory_order_seq_cst);
            size_t cur_tail = tail_.load(std::memory_order_seq_cst);
            if (((cur_head - cur_tail) & mask) == mask) {
                return false; 
            }
            buffer_[cur_head & mask] = item;
            head_.store(cur_head + 1, std::memory_order_seq_cst);
            return true;

        };
        bool try_pop(T& item){
            size_t cur_tail = tail_.load(std::memory_order_seq_cst);
            if (cur_tail == head_.load(std::memory_order_seq_cst)) {
                return false; // Buffer is empty
            }
            cur_tail = tail_.fetch_add(1, std::memory_order_seq_cst);  // returns old value, then adds 1
            item = buffer_[cur_tail & mask];
            return true;
           
        };
        bool is_empty() const {
            return head_.load(std::memory_order_seq_cst) == tail_.load(std::memory_order_seq_cst);
        };
        bool is_full() const {
            return ((head_.load(std::memory_order_seq_cst) - tail_.load(std::memory_order_seq_cst)) & mask) == mask;
        };
    
    private:
        static constexpr size_t mask = Capacity - 1;
        alignas(64) std::atomic<size_t> head_{0};
        alignas(64) std::atomic<size_t> tail_{0};
        T buffer_[Capacity];
};