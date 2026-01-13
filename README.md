# High-Performance Async Logger

A low-latency, lock-free asynchronous logging library built in C++ for high-frequency trading (HFT) applications. Features a cache-optimized SPSC ring buffer and acquire-release memory ordering for minimal critical path overhead.

## Features

- **Lock-free SPSC ring buffer** with cache-line alignment to prevent false sharing
- **Sub-20ns logging latency** on critical path
- **Asynchronous I/O** - background thread handles disk writes
- **Drop-on-full policy** - never blocks the caller
- **Nanosecond timestamp precision**
- **Memory ordering optimizations** - 2.1x throughput improvement over sequential consistency

## Performance

### Critical Path Latency
```
Logger.log() with fixed string:  20.2 ns  (49.5M logs/sec)
Logger.log() with allocation:    25.9 ns  (38.6M logs/sec)
```

### RingBuffer Performance
| Metric | Sequential Consistency | Acquire-Release | Improvement |
|--------|------------------------|-----------------|-------------|
| Single Push | 2.10 ns | 1.96 ns | 7% faster |
| SPSC Throughput | 71.7M/s | 152M/s | **2.1x faster** |

### Multi-threaded Performance
- **1 producer:**  111.6M logs/sec
- **2 producers:** 151.0M logs/sec (peak)
- **4 producers:** 55.9M logs/sec
- **8 producers:** 111.7M logs/sec

*Tested on Apple M-series (ARM64), 10 cores*

## Architecture
```
Application Thread              Background Thread
      |                               |
      | log("message")               |
      |                               |
      | 1. Create LogEntry           |
      | 2. Add timestamp             |
      | 3. try_push() → RingBuffer  |
      |     (< 20ns)                 |
      |                               |
      |                         [RingBuffer]
      |                          1024 entries
      |                         Cache-aligned
      |                               |
      |                          try_pop()
      |                               |
      |                          Format + Write
      |                          to disk (100μs)
      |                               |
   Returns immediately           Continues...
```

### Key Design Decisions

1. **Lock-free SPSC Queue**
   - Single producer, single consumer model
   - No mutex contention on critical path
   - Acquire-release memory ordering for ARM optimization

2. **Cache Line Alignment**
```cpp
   alignas(64) std::atomic<size_t> head_;
   alignas(64) std::atomic<size_t> tail_;
```
   - Prevents false sharing between producer and consumer
   - 64-byte alignment matches CPU cache line size

3. **Drop-on-Full Policy**
   - Never blocks the caller (HFT requirement)
   - Tracks dropped count for monitoring
   - Alternative: increase buffer size or add backpressure

4. **Fixed-Size Log Entries**
```cpp
   struct LogEntry {
       char message[512];
       size_t length;
       uint64_t timestamp;
   };
```
   - Zero heap allocation on critical path
   - Predictable memory usage
   - Cache-friendly (inline data)

## Build Instructions

### Prerequisites
```bash
# macOS
brew install cmake

# Ubuntu/Debian
sudo apt install cmake build-essential
```

### Clone and Build
```bash
git clone https://github.com/YOUR_USERNAME/async-logger.git
cd async-logger
git submodule update --init --recursive  # For Google Benchmark

mkdir build && cd build
cmake ..
make
```

### Run Tests
```bash
./ring_buffer_test          # Single-threaded correctness
./ring_buffer_mt_test       # Multi-threaded stress test
./logger_test               # Logger functionality
```

### Run Benchmarks
```bash
./ring_buffer_benchmark     # RingBuffer performance
./logger_benchmark          # Logger performance
./compare_memory_ordering   # Before/after optimization
```

## Usage

### Basic Example
```cpp
#include "logger.hpp"

int main() {
    Logger logger("app.log");
    
    logger.log("Application started");
    logger.log("Processing order 12345");
    logger.log("Trade executed");
    
    // Destructor automatically flushes all pending logs
    return 0;
}
```

### Production Example
```cpp
#include "logger.hpp"

int main() {
    Logger logger("trading.log");
    
    // High-frequency trading loop
    for (int i = 0; i < 1000000; i++) {
        // Critical path - only ~20ns overhead
        logger.log("Market data update: " + std::to_string(i));
        
        // Your trading logic here...
    }
    
    // Check if any logs were dropped
    uint64_t dropped = logger.get_dropped_count();
    if (dropped > 0) {
        std::cerr << "Warning: " << dropped << " logs dropped\n";
    }
    
    return 0;
}
```

### Integration with Your Project
```cmake
# CMakeLists.txt
add_subdirectory(path/to/async-logger)
target_link_libraries(your_app logger)
```

## Project Structure
```
async-logger/
├── include/
│   ├── ring_buffer.hpp      # Lock-free SPSC queue
│   └── logger.hpp            # Async logger interface
├── src/
│   └── logger.cpp            # Logger implementation
├── tests/
│   ├── ring_buffer_test.cpp
│   ├── ring_buffer_mt_test.cpp
│   └── logger_test.cpp
├── benchmarks/
│   ├── ring_buffer_benchmark.cpp
│   ├── logger_benchmark.cpp
│   └── compare_memory_ordering.cpp
└── CMakeLists.txt
```

## Memory Ordering Optimization

### Before (Sequential Consistency)
```cpp
head_.load(std::memory_order_seq_cst);
head_.store(value, std::memory_order_seq_cst);
```
- Strongest ordering guarantee
- Most expensive (memory barriers on every operation)
- Result: 71.7M ops/sec on ARM

### After (Acquire-Release)
```cpp
// Producer
buffer_[head & MASK] = item;
head_.store(head + 1, std::memory_order_release);

// Consumer
size_t h = head_.load(std::memory_order_acquire);
item = buffer_[tail & MASK];
```
- Provides sufficient synchronization for SPSC
- Much cheaper on ARM architecture
- Result: 152M ops/sec on ARM (**2.1x improvement**)

### Why It Works
- `release` ensures buffer write completes before head update
- `acquire` ensures consumer sees all producer writes
- Creates "synchronizes-with" relationship
- No unnecessary global ordering

## Limitations & Future Work

### Current Limitations
- Single consumer (background thread is the bottleneck)
- Fixed 512-byte message size (truncates longer messages)
- No log levels (INFO/WARN/ERROR)
- No log rotation
- Basic timestamp formatting

### Potential Improvements
- [ ] Add formatted logging (fmt-style: `log("value: {}", x)`)
- [ ] Implement log levels with filtering
- [ ] Add log rotation policies
- [ ] Support multiple log files
- [ ] Configurable drop vs. block policy
- [ ] Binary logging format (skip formatting entirely)
- [ ] MPMC queue for multiple producers
- [ ] Batch writes for higher throughput

## Testing Methodology

### Correctness Testing
1. **Single-threaded tests** - Basic operations, edge cases, wraparound
2. **Multi-threaded stress tests** - 1M+ operations, verify no data loss
3. **High contention tests** - Small buffer, forces blocking

### Performance Testing
1. **Latency measurement** - CPU cycle counters (TSC/ARM timer)
2. **Throughput testing** - Sustained load over time
3. **Multi-threaded scaling** - 1, 2, 4, 8 producers
4. **Drop rate analysis** - Behavior under extreme load

## Technical Deep Dive

### False Sharing Prevention
Without alignment:
```
[head_][tail_] ← Same 64-byte cache line
```
- Every head update invalidates tail's cache line
- Consumer experiences cache miss on every read
- Result: 100-200ns per operation

With alignment:
```
[head_][padding...] ← Cache line 0
[tail_][padding...] ← Cache line 1
```
- Producer owns cache line 0
- Consumer owns cache line 1
- No interference: ~10-20ns per operation

### Power-of-2 Sizing
```cpp
static_assert((Capacity & (Capacity - 1)) == 0, "Must be power of 2");
index = (index + 1) & MASK;  // Fast bitwise AND
// vs
index = (index + 1) % Capacity;  // Slow division
```

## License

MIT License - see LICENSE file for details

## Author

[Your Name]
- GitHub: [@yourusername](https://github.com/yourusername)
- Built as part of learning HFT systems programming

## Acknowledgments

- Inspired by production HFT logging systems
- Memory ordering concepts from [Jeff Preshing's blog](https://preshing.com/)
- Lock-free queue design patterns from [1024cores](http://www.1024cores.net/home/lock-free-algorithms)

---

**Note:** This is an educational project demonstrating low-latency C++ programming techniques. For production HFT use, consider battle-tested libraries like spdlog or NanoLog.
