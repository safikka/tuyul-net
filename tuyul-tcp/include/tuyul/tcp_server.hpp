#ifndef TUYUL_TCP_SERVER_HPP
#define TUYUL_TCP_SERVER_HPP

#include <tuyul/error_code.hpp>
#include <tuyul/tcp_options.hpp>
#include <functional>
#include <string_view>
#include <memory>

namespace tuyul {

class TcpServer {
public:
    // Callback types for loose coupling and standard logging separation
    using DataCallback = std::function<void(int client_fd, std::string_view raw_payload)>;
    using LoggerCallback = std::function<void(std::string_view log_level, std::string_view log_message)>;

    // Explicit constructor ensuring port allocation rules
    explicit TcpServer(int port, TcpOptions options = TcpOptions{});
    
    // Core RAII Destructor to ensure proper file descriptor cleanup on teardown
    ~TcpServer();

    // Delete copy semantics to enforce strict network resource ownership
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    // Allow move semantics for ownership transferring
    TcpServer(TcpServer&&) noexcept;
    TcpServer& operator=(TcpServer&&) noexcept;

    /**
     * @brief Registers the custom operational logger callback.
     */
    void set_logger(LoggerCallback logger);

    /**
     * @brief Registers the trap callback to intercept zero-copy network packets.
     */
    void on_data(DataCallback callback);

    /**
     * @brief Spawns the socket environment and starts the synchronous loop execution.
     * @return ErrorCode detailing the operational boot status.
     */
    ErrorCode start();

    /**
     * @brief Forcefully triggers shutdown procedures and clears internal descriptors.
     */
    void stop();

private:
    // Forward declaration of internal Implementation details (Pimpl Pattern)
    // This keeps OS-specific headers (like sys/socket.h, epoll.h) out of our public headers
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tuyul

#endif // TUYUL_TCP_SERVER_HPP
