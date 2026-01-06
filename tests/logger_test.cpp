#include <iostream>
#include <thread>
#include <chrono>
#include "../include/logger.hpp"

int main() {
    std::cout << "Testing Logger...\n";
    
    {
        Logger logger("test.log");
        
        // Test 1: Basic logging
        std::cout << "Test 1: Basic logging\n";
        logger.log("Logger started");
        logger.log("This is a test message");
        logger.log("Testing 123");
        
        // Give background thread time to write
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Test 2: Rapid logging
        std::cout << "Test 2: Rapid logging (10000 messages)\n";
        for (int i = 0; i < 10000; i++) {
            logger.log("Message " + std::to_string(i));
        }
        
        std::cout << "Waiting for flush...\n";
        // Destructor will wait for all logs to be written
    }
    
    std::cout << "Logger destroyed, check test.log\n";
    std::cout << "✅ Test complete\n";
    
    return 0;
}