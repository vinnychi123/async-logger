#include "logger.hpp"
#include <iostream>

Logger::Logger(const std::string& filename, size_t buffer_size)
    : shutdown_flag_(false), dropped_count_(0){
    log_file_.open(filename, std::ios::out | std::ios::app);
    if (!log_file_.is_open()){
        throw std::runtime_error("Failed to open log file");

    }

    background_thread_ = std::thread(&Logger::background_worker, this);

}

Logger::~Logger(){
    shutdown_flag_.store(true);

    if(background_thread_.joinable()){
        background_thread_.join();
    }
    
    if(log_file_.is_open()){
        log_file_.close();
    }

    uint64_t dropped = dropped_count_.load();
    if (dropped > 0){
        std::cerr << "Logger: Dropped " << dropped << " log entries.\n";
    }
 
}

void Logger::log(const std::string& message){
    LogEntry entry;

    entry.timestamp = get_timestamp_ns();

    size_t len = std::min(message.size(), sizeof(entry.message) - 1);
    std::memcpy(entry.message, message.c_str(), len);
    entry.message[len] = '\0';
    entry.length = len;

    if(!ring_buffer_.try_push(entry)){
        dropped_count_.fetch_add(1, std::memory_order_relaxed);
    }

}

void Logger::background_worker(){
    LogEntry entry;

    while(!shutdown_flag_.load(std::memory_order_acquire)){
        if(ring_buffer_.try_pop(entry)){
            log_file_ << format_log_entry(entry) << std::endl;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    while (ring_buffer_.try_pop(entry)){
        log_file_ << format_log_entry(entry) << std::endl;
    }

    log_file_.flush();

}

uint64_t Logger::get_timestamp_ns(){
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

std::string Logger::format_log_entry(const LogEntry& entry){

    return "[" + std::to_string(entry.timestamp) + "] " + std::string(entry.message, entry.length);
}