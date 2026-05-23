#include <tuyul/tcp_client.hpp>
#include <tuyul/tcp_options.hpp>
#include <tuyul/error_code.hpp>
#include <iostream>
#include <fstream>
#include <string_view>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <csignal>
#include <thread>

// Global pointer to allow the standalone signal handler to access our client instance
static tuyul::TcpClient* global_client_ptr = nullptr;
static bool keep_running = true;

/**
 * @brief Custom OS Signal Intercept handler to execute graceful client disconnection.
 */
void signal_handler(int signal_number) {
    if (signal_number == SIGINT) {
        std::cout << "\n[APP-WARN] Intercepted Ctrl+C (SIGINT). Stopping interaction loop..." << std::endl;
        keep_running = false;
        if (global_client_ptr != nullptr) {
            global_client_ptr->disconnect();
        }
    }
}

static std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

int main() {
    std::cout << "=============================\n";
    std::cout << "TUYUL-NET: TCP CLIENT EXAMPLE\n";
    std::cout << "=============================\n" << std::endl;

    // Setup localized file logging pipeline for the client activity
    const std::string log_filename = "tuyul_client_activity.log";
    std::ofstream log_file(log_filename, std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "[APP-ERROR] Failed to initialize client log file stream: " << log_filename << std::endl;
        return 1;
    }
    std::cout << "[APP-INFO] Client activity log stream hooked to: ./" << log_filename << std::endl;

    // Tune custom core client options parameters dynamically
    tuyul::TcpOptions custom_config;
    custom_config.network_buffer_size = 4096;   // 4KB buffer for reading server replies
    custom_config.max_epoll_events = 16;        // Small queue bound is sufficient for a single client link
    custom_config.watchdog_timeout_ms = 1000;   // Check background state changes quickly (every 1 second)

    // Spawning the client instance and linking signals
    tuyul::TcpClient client(custom_config);
    global_client_ptr = &client;
    std::signal(SIGINT, signal_handler);

    // Inject the custom file logger callback framework
    client.set_logger([&log_file](std::string_view log_level, std::string_view log_message) {
        log_file << "[" << get_current_timestamp() << "] [" << log_level << "] " << log_message << "\n";
        log_file.flush();
    });

    // Setup zero-copy response handling callback to read server replies
    client.on_data([](std::string_view server_reply) {
        std::cout << "\n📥 [APP-EVENT] Received data reply back from remote server:\n";
        std::cout << "------------------------------------------------------------------------\n";
        std::cout << server_reply;
        std::cout << "------------------------------------------------------------------------\n" << std::endl;
    });

    // Initiate asynchronous non-blocking transport link connection to local server
    const std::string target_host = "127.0.0.1";
    const int target_port = 7575;
    std::cout << "[APP-INFO] Attaching link channel connection to " << target_host << ":" << target_port << "..." << std::endl;

    tuyul::ErrorCode conn_status = client.connect_to(target_host, target_port);
    if (conn_status != tuyul::ErrorCode::SUCCESS) {
        std::cerr << "[APP-FATAL] Failed to connect to server! Boot Failure Code: " << static_cast<int>(conn_status) << std::endl;
        return 1;
    }

    // Interactive loop: send heartbeats down the link pipeline channel every few seconds
    std::cout << "[APP-INFO] Connection linked! Sending data packets. Press Ctrl+C to terminate cleanly.\n" << std::endl;
    
    int loop_counter = 1;
    while (keep_running) {
        std::string payload = "Ping Packet #" + std::to_string(loop_counter++) + " from Tuyul-Net Client\n";
        
        std::cout << "📤 [SEND] Dispatching payload down the wire: " << payload;
        tuyul::ErrorCode send_status = client.send_data(payload);
        
        if (send_status != tuyul::ErrorCode::SUCCESS) {
            std::cerr << "[APP-ERROR] Transmission aborted! Link channel might have dropped natively." << std::endl;
            break;
        }

        // Sleep the main thread execution context for 3 seconds before next cycle iteration
        for (int i = 0; i < 30 && keep_running; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // Explicit fallback cleanup execution upon exiting the main loop context safely
    client.disconnect();
    global_client_ptr = nullptr;

    std::cout << "[APP-INFO] Client sandbox application cleanly exited. Good bye!" << std::endl;
    return 0;
}
