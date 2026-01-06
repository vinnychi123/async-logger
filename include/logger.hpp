#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <fstream>
#include <chrono>
#include <cstring>
#include "ring_buffer.hpp"

class Logger {
    public:
        Logger(const std::string& filename, size_t buffer_size = 1024);
        ~Logger();

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        void log(const std::string& message);

        uint64_t get_dropped_count() const {return dropped_count_.load();}

    private:
        struct LogEntry{
            char message[512];
            size_t length;
            uint64_t timestamp;
        };

        RingBuffer<LogEntry, 1024> ring_buffer_;
        std::thread background_thread_;
        std::atomic<bool> shutdown_flag_;
        std::atomic<uint64_t> dropped_count_;
        std::ofstream log_file_;

        void background_worker();
        uint64_t get_timestamp_ns();
        std::string format_log_entry(const LogEntry& entry);
};