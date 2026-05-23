#include <tuyul/tcp_server.hpp>
#include <tuyul/tcp_options.hpp>
#include <tuyul/error_code.hpp>
#include <iostream>
#include <fstream>
#include <string_view>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <csignal>

// Global pointer wrapper to allow the standalone signal handler to access our server instance
static tuyul::TcpServer* global_server_ptr = nullptr;

/**
 * @brief Custom OS Signal Intercept handler to execute graceful shutdown routines.
 */
void signal_handler(int signal_number) {
    if (signal_number == SIGINT && global_server_ptr != nullptr) {
        std::cout << "\n[APP-WARN] Intercepted Ctrl+C (SIGINT). Initiating graceful shutdown sequences..." << std::endl;
        
        // Forcefully break the epoll loop and trigger internal RAII cleanup routines gracefully
        global_server_ptr->stop();
    }
}

/**
 * @brief Helper utility to generate ISO 8601 synchronized timestamp strings.
 */
static std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

int main() {
    std::cout << "=============================\n";
    std::cout << "TUYUL-NET: TCP SERVER EXAMPLE\n";
    std::cout << "=============================\n" << std::endl;

    // Setup localized file logging pipeline
    const std::string log_filename = "tuyul_net_activity.log";
    std::ofstream log_file(log_filename, std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "[APP-ERROR] Failed to initialize target log file stream: " << log_filename << std::endl;
        return 1;
    }
    std::cout << "[APP-INFO] Activity log stream successfully hooked to: ./" << log_filename << std::endl;

    // Tune custom core options parameters dynamically
    tuyul::TcpOptions custom_config;
    custom_config.network_buffer_size = 8192;   // Allocate 8KB data chunks memory
    custom_config.max_epoll_events = 128;       // Expand epoll wait loop queue size
    custom_config.watchdog_timeout_ms = 2000;   // Wake up interval every 2 seconds

    // Spawning the server instance mapping
    const int target_port = 7575;
    tuyul::TcpServer server(target_port, custom_config);
    
    // Bind the local instance address to our global pointer for signal intercept awareness
    global_server_ptr = &server;

    // Register the native OS signal listener hook points
    std::signal(SIGINT, signal_handler);
    std::cout << "[APP-INFO] Attached SIGINT signal registration listener hook." << std::endl;
    std::cout << "[APP-INFO] Initializing asynchronous TCP reactor core on port " << target_port << "..." << std::endl;

    // Inject the custom file logger callback framework
    server.set_logger([&log_file](std::string_view log_level, std::string_view log_message) {
        log_file << "[" << get_current_timestamp() << "] [" << log_level << "] " << log_message << "\n";
        log_file.flush(); // Ensure atomic disk persistence even during abrupt software interruptions
    });

    // Setup zero-copy data interception trap callback
    server.on_data([](int client_fd, std::string_view raw_payload) {
        std::cout << "\n[APP-EVENT] Intercepted payload packet from client descriptor (" << client_fd << "):\n";
        std::cout << "------------------------------------------------------------------------\n";
        std::cout << raw_payload;
        std::cout << "------------------------------------------------------------------------\n" << std::endl;
    });

    // Execute core network loop sequence
    tuyul::ErrorCode execution_status = server.start();

    // Clean up global references upon loop completion exit
    global_server_ptr = nullptr;

    if (execution_status != tuyul::ErrorCode::SUCCESS) {
        if (execution_status == tuyul::ErrorCode::BIND_FAILED) {
            std::cerr << "[APP-FATAL] Port " << target_port << " is blocked by another process!\n";
        } else {
            std::cerr << "[APP-FATAL] Unhandled core runtime failure code: " 
                      << static_cast<int>(execution_status) << "\n";
        }
        return 1;
    }

    std::cout << "[APP-INFO] Application server successfully exited with clean statuses. Good bye!" << std::endl;
    return 0;
}
